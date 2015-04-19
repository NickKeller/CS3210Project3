#include <mysql/mysql.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mysql/my_global.h>
#include <mysql/my_config.h>

void deleteImage(MYSQL *con, char path[]);
void finish_with_error(MYSQL *con);
void updatePath(MYSQL *con, char newPath[], char oldPath[]);
void insertImage(MYSQL *con, char path[]);

void finish_with_error(MYSQL *con)
{
  fprintf(stderr, "%s\n", mysql_error(con));
  mysql_close(con);
  exit(1);        
}

int main(int argc, char **argv)
{
  MYSQL *con = mysql_init(NULL);
  
  if (con == NULL) 
  {
      fprintf(stderr, "%s\n", mysql_error(con));
      exit(1);
  }  

  if (mysql_real_connect(con, "pujaridb.csciwcfdboyu.us-east-1.rds.amazonaws.com", "cs3210", "12345678", 
          "rpfs", 0, NULL, 0) == NULL) 
  {
      finish_with_error(con);
  }    
  
  insertImage(con, "blah.png");

  mysql_close(con);
  exit(0);
}

void deleteImage(MYSQL *con, char path[]){

  MYSQL *con = mysql_init(NULL);
  
  if (con == NULL) 
  {
      fprintf(stderr, "%s\n", mysql_error(con));
      exit(1);
  }  

  if (mysql_real_connect(con, "pujaridb.csciwcfdboyu.us-east-1.rds.amazonaws.com", "cs3210", "12345678", 
          "rpfs", 0, NULL, 0) == NULL) 
  {
      finish_with_error(con);
  }

  char src[50], query[100], end[50];

  strcpy(query, "DELETE from Images WHERE Path='");
  strcpy(src,  path);
  strcpy(end,"';");
  
  strcat(query, src);
  strcat(query,end);

  if (mysql_query(con, query)) {
      finish_with_error(con);
  }

  mysql_close(con);
  exit(0);
}

void updatePath(char newPath[], char oldPath[]){
  
  MYSQL *con = mysql_init(NULL);
  
  if (con == NULL) 
  {
      fprintf(stderr, "%s\n", mysql_error(con));
      exit(1);
  }  

  if (mysql_real_connect(con, "pujaridb.csciwcfdboyu.us-east-1.rds.amazonaws.com", "cs3210", "12345678", 
          "rpfs", 0, NULL, 0) == NULL) 
  {
      finish_with_error(con);
  }    

  char query[100], end[50], mid[50];

  strcpy(query, "UPDATE Images SET Path='");
  strcpy(mid,"'WHERE Path='");
  strcpy(end,"';");
  
  strcat(query, newPath);
  strcat(query,mid);
  strcat(query,oldPath);
  strcat(query,end);

  if (mysql_query(con,query)) {
      finish_with_error(con);
  }

  mysql_close(con);
  exit(0);
}

void insertImage(char path[]){
  MYSQL *con = mysql_init(NULL);
  
  if (con == NULL) 
  {
      fprintf(stderr, "%s\n", mysql_error(con));
      exit(1);
  }  

  if (mysql_real_connect(con, "pujaridb.csciwcfdboyu.us-east-1.rds.amazonaws.com", "cs3210", "12345678", 
          "rpfs", 0, NULL, 0) == NULL) 
  {
      finish_with_error(con);
  }

  FILE *fp = fopen(path, "rb");
  
  if (fp == NULL) 
  {
      fprintf(stderr, "cannot open image file\n");    
      exit(1);
  }
      
  fseek(fp, 0, SEEK_END);
  
  if (ferror(fp)) {
      
      fprintf(stderr, "fseek() failed\n");
      int r = fclose(fp);

      if (r == EOF) {
          fprintf(stderr, "cannot close file handler\n");          
      }    
      
      exit(1);
  }  
  
  int flen = ftell(fp);
  
  if (flen == -1) {
      perror("error occurred");
      int r = fclose(fp);

      if (r == EOF) {
          fprintf(stderr, "cannot close file handler\n");
      }
      exit(1);      
  }
  
  fseek(fp, 0, SEEK_SET);
  
  if (ferror(fp)) {
      
      fprintf(stderr, "fseek() failed\n");
      int r = fclose(fp);

      if (r == EOF) {
          fprintf(stderr, "cannot close file handler\n");
      }    
      exit(1);
  }

  char data[flen+1];

  int size = fread(data, 1, flen, fp);
  
  if (ferror(fp)) {
      
      fprintf(stderr, "fread() failed\n");
      int r = fclose(fp);

      if (r == EOF) {
          fprintf(stderr, "cannot close file handler\n");
      }
      exit(1);      
  }
  
  int r = fclose(fp);
  if (r == EOF) {
      fprintf(stderr, "cannot close file handler\n");
  }          
    
  char chunk[2*size+1];
  mysql_real_escape_string(con, chunk, data, size);

  char st[] = "INSERT INTO Images(Path, Image) VALUES('";
  
  strcat(st,path);
  strcat(st, "','%s');");
  
  size_t st_len = strlen(st);

  char query[st_len + 2*size+1]; 
  int len = snprintf(query, st_len + 2*size+1, st, chunk);

  if (mysql_real_query(con, query, len))
  {
      finish_with_error(con);
  }

  mysql_close(con);
  exit(0);
}