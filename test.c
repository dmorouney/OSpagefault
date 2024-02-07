#include <stdio.h>
#include <stdlib.h>


typedef struct node 
{
    int id;
    struct node *next;
}node_t;

typedef struct listt 
{
    int items;
    node_t *head;
} list_t;

list_t * init_list()
{
    return malloc(sizeof(list_t));
}
void add_list(list_t *lst, int id)
{
    node_t *cur = lst->head;
    while(1)
    {
    if(cur == NULL)
    {
        cur = malloc(sizeof(node_t));
        cur->id = id;
        cur->next = lst->head;
        lst->head = cur;
        lst->items++;
        printf("adding id to list\n");
        break;
    }
    else if (cur->id == id)
    {
        printf("id already in list\n");
        break;
    }
    else
    {
        printf("Continuing...\n");
        cur = cur->next;
        continue;
    }
    }
}
int main()
{
    list_t *list = init_list();
    add_list(list, 2);
    add_list(list, 1);
    add_list(list, 3);
    add_list(list, 2);
    printf("The number of items in the list is: %d\n", list->items);
}
