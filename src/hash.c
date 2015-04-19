#include <stdio.h>
#include <stdlib.h>

unsigned long hashFunction(char *str);
void addNode(char *mount);
void removeNode(char *mount);
struct node *search(char *key);
void printList();

struct node
{
    unsigned long hash;
    char *mount;
    struct node *next;
};

struct node *head;
int size = 0;


void addNode(char *mount)
{
    unsigned long hash = hashFunction(mount);
    if (size == 0)
    {
        head = (struct node *) malloc(sizeof(struct node));
        head->hash = hash;
        head->mount = mount;
        head->next = head;
    }
    else if (strncmp(head->mount, mount) == 0)
    {
        return;
    }
    else
    {
        if (hash < head->hash)  // less than head
        {
            printf("Here\n");
            struct node *newNode = (struct node *) malloc(sizeof(struct node));
            newNode->next = head->next;
            head->next = newNode;
            newNode->hash = head->hash;
            newNode->mount = head->mount;
            head->hash = hash;
            head->mount = mount;
        }
        else    // greater than head
        {
            struct node *curr = head;

            while (curr->next != head) // tail
            {
                if (strncmp(curr->next->mount, mount) == 0)   // mount exists
                    return;
                if (hash < curr->next->hash)
                    break;
                curr = curr->next;
            }

            struct node *newNode = (struct node *) malloc(sizeof(struct node));
            newNode->hash = hash;
            newNode->mount = mount;
            newNode->next = curr->next;
            curr->next = newNode;
        }
    }
    size++;
}


void removeNode(char *mount)
{
    if (size == 1)
    {
        head = NULL;
    }
    else if (strncmp(head->mount, mount) == 0)
    {
        struct node *headNext = head->next;
        head->hash = headNext->hash;
        head->mount = headNext->mount;
        head->next = headNext->next;
    }
    else
    {
        struct node *curr = head;
        while (curr->next != head)
        {
            if (strncmp(curr->next->mount, mount) == 0)
                break;
            curr = curr->next;
        }
        if (curr->next == head)     // not found
            return;
        curr->next = curr->next->next;
    }
    size--;
}


struct node *search(char *key)
{
    unsigned long hash = hashFunction(key);
    struct node *curr = head;
    if (hash < curr->hash)
    {
        return curr;
    }
    else
    {
        while (curr->next != head)
        {
            if (hash < curr->next->hash)
                return curr;
            curr = curr->next;
        }
        return head;
    }
}


unsigned long hashFunction(char *str)
{
    unsigned long hash = 0;
    int c;

    while (c = *str++)
        hash = c + (hash << 6) + (hash << 16) - hash;

    return hash;
}


void printList()
{
    struct node *curr = head;
    int i;
    printf("Head\n");
    for (i = 0; i < size; i++)
    {
        printf("%lu  ==  %s\n", curr->hash, curr->mount);
        curr = curr->next;
    }

}
