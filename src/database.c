#include <mysql/mysql.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mysql/my_config.h>
#include <mysql/my_global.h>

int deleteImage(char path[]);
int updatePath( char newPath[], char oldPath[]);
int insertImage(char *path);

int deleteImage(char path[]){

	int success=1;
	if(success==1){
	MYSQL *con = mysql_init(NULL);
	  
	if (con == NULL){
	    success=0;
	}  

	if (mysql_real_connect(con, "pujaridb.csciwcfdboyu.us-east-1.rds.amazonaws.com", "cs3210", "12345678", 
	    "rpfs", 0, NULL, 0) == NULL) 
	{
	   success=0;
	}

	char src[50], query[100], end[50];

	strcpy(query, "DELETE from Images WHERE Path='");
	strcpy(src,  path);
	strcpy(end,"';");
  
  	strcat(query, src);
  	strcat(query,end);

  	
	if (mysql_query(con, query)) {
	  success=0;
	}
	
  mysql_close(con);
}
  return success;
}

int updatePath(char newPath[], char oldPath[]){
  int success=1;
  if(success==1){
  MYSQL *con = mysql_init(NULL);
  
  if (con == NULL) 
  {
    success=0;
  }  

  if (mysql_real_connect(con, "pujaridb.csciwcfdboyu.us-east-1.rds.amazonaws.com", "cs3210", "12345678", 
          "rpfs", 0, NULL, 0) == NULL) 
  {
	success=0;      
  }    

  char query[100], end[50], mid[50];

  strcpy(query, "UPDATE Images SET Path='");
  strcpy(mid,"'WHERE Path='");
  strcpy(end,"';");
  
  strcat(query, newPath);
  strcat(query,mid);
  strcat(query,oldPath);
  strcat(query,end);

  if(mysql_query(con,query)) {
     success=0;
  }

  mysql_close(con);
}
  return success;
}

int insertImage(char *path){

  int success=1;
  if(success){
  	fprintf(stderr,"mysql_init\n");
  MYSQL *con = mysql_init(NULL);
  fprintf(stderr,"mysql_init done\n");
  
  if (con == NULL) 
  {
    success=0;
  }  
fprintf(stderr,"mysql_connect begin\n");
  if (mysql_real_connect(con, "pujaridb.csciwcfdboyu.us-east-1.rds.amazonaws.com", "cs3210", "12345678", 
          "rpfs", 0, NULL, 0) == NULL) 
	{
	  success=0;
	}
fprintf(stderr,"mysql_connect done\n");
	int lenStr = strlen(path)*2;
	char buffer[lenStr];
	int j= 0;

	for( size_t i = 0; i < strlen(path); i++ )
	{ 
	    buffer[j] = path[i];
	    if(path[i]=='/'){
	      j++;
	      buffer[j]='/';
	    }
	    j++;
	}
	buffer[j]='\0';

	  FILE *fp = fopen(buffer, "rb");
	  
	  if (fp == NULL) 
	  {
	      success=0;
	  }
	      
	  fseek(fp, 0, SEEK_END);
	  
	  if (ferror(fp)) {
	  	success=0;
	  }  
	  
	  int flen = ftell(fp);
	  
	  if (flen == -1) {
	  	success=0;
	  }
	  
	  fseek(fp, 0, SEEK_SET);
	  
	  if (ferror(fp)) {
		success=0;
	  }

	  char data[flen+1];

	  int size = fread(data, 1, flen, fp);
	  
	  if (ferror(fp)) {
	      success=0;   
	  }
	  
	  int r = fclose(fp);
	  if (r == EOF) {
	      fprintf(stderr, "cannot close file handler\n");
	  }          
	    
	  char chunk[2*size+1];
	  mysql_real_escape_string(con, chunk, data, size);

	 char st1[] = "INSERT INTO Images(Path, Image) VALUES('";
	  //printf("Doing string concatenation\n");
	  char* st = calloc(1000,sizeof(char));
	  strcpy(st,st1);
	  strcat(st,path);
	  strcat(st, "','%s');");
	  
	  size_t st_len = strlen(st);

	  char query[st_len + 2*size+1]; 
	  int len = snprintf(query, st_len + 2*size+1, st, chunk);
	  fprintf(stderr,"query begin\n");
	  if (mysql_real_query(con, query, len))
	  {
	    success=0;
	  }
fprintf(stderr,"query done\n");
	  mysql_close(con);
	  fprintf(stderr,"Closed connection\n");
	}
	//fclose(logfile);
 return success;
}
