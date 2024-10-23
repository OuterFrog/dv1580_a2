#include "linked_list.h"

pthread_rwlock_t rw_lock;

// Initializes the list by intiliazing the memory_manager with a memory pool of 'size'
void list_init(Node** head, size_t size){
    pthread_rwlock_init(&rw_lock, NULL);

    mem_init(size);
    *head = NULL;
}

// Inserts new node with data of 'data' at the end of list
void list_insert(Node** head, uint16_t data){
    pthread_rwlock_wrlock(&rw_lock);

    Node* new_node = mem_alloc(sizeof(Node));
    if(new_node == 0){
        // Can't allocate new node
        printf("ERROR!");
        pthread_rwlock_unlock(&rw_lock);
        return;
    }
    new_node->data = data;
    new_node->next = NULL;

    if(*head == NULL){
        *head = new_node;
    }
    else{
        // Walk 'til end of list, then insert
        Node* walker = *head;
        while(walker->next != NULL){
            walker = walker->next;
        }

        walker->next = new_node;
    }
    
    // Node* new_node = mem_alloc(sizeof(Node));
    // if(new_node == 0){
    //     printf("ERROR!");
    // }
    // new_node->data = data;
    // new_node->next = *head;
    // *head = new_node;

    pthread_rwlock_unlock(&rw_lock);
}

// Inserts a new node inbetween prev_node and prev_node->next
void list_insert_after(Node* prev_node, uint16_t  data){
    pthread_rwlock_wrlock(&rw_lock);

    Node* new_node = mem_alloc(sizeof(Node));
    if(new_node == 0){
        // Can't allocate new node
        printf("ERROR!");
        pthread_rwlock_unlock(&rw_lock);
        return;
    }
    new_node->data = data;
    new_node->next = prev_node->next;
    prev_node->next = new_node;

    pthread_rwlock_unlock(&rw_lock);
}

// Insert node with data 'data' before node 'next_node'. 
// Uses 'head' to start traversing through the list
void list_insert_before(Node** head, Node* next_node, uint16_t  data){
    pthread_rwlock_wrlock(&rw_lock);

    Node* new_node = mem_alloc(sizeof(Node));
        if(new_node == 0){
        // Can't allocate new node
        printf("ERROR!");
        pthread_rwlock_unlock(&rw_lock);
        return;
    }
    new_node->data = data;
    
    if(next_node == *head){
        //Insert before 'head', which means just insert at beginning of list
        new_node->next = *head;
        *head = new_node;
    }
    else{
        // Traverse through the list until it finds the node before 'next_node'
        Node* walker = *head;
        while(walker->next != NULL && walker->next != next_node){
            walker = walker->next;
        }

        // Then make new node point to 'next node',
        // and the node previously pointing to 'next node', to instead point to new node.
        new_node->next = next_node;
        walker->next = new_node;
    }

    pthread_rwlock_unlock(&rw_lock);
}

// Delete the first node that contains 'data'
void list_delete(Node** head, uint16_t  data){
    pthread_rwlock_wrlock(&rw_lock);
    if(*head == NULL){
        printf("ERROR: Deleting from empty list!");
        pthread_rwlock_unlock(&rw_lock);
        return;
    }
    
    if((*head)->data == data){
        // If the first node has 'data', simply change what head points toward
        Node* toDel = *head;
        *head = (*head)->next;

        mem_free(toDel);
    }
    else{
        // Otherwise traverse through the list to find it
        Node* walker = *head;
        while(walker->next != NULL && walker->next->data != data){
            walker = walker->next;

            if(walker->next != NULL){
                Node* toDel = walker->next;
                walker->next = toDel->next;

                mem_free(toDel);
            }
        }
    }

    pthread_rwlock_unlock(&rw_lock);
}

// Searches the list to find node containing 'data'
Node* list_search(Node** head, uint16_t  data){
    pthread_rwlock_rdlock(&rw_lock);

    // If the list is empty, immediately return NULL
    if(head == NULL){
        printf("ERROR: head is null");
        pthread_rwlock_unlock(&rw_lock);
        return NULL;
    }
    
    // Traverses through the list
    Node* walker = *head;
    while(walker != NULL && walker->data != data){
        walker = walker->next;
    }

    if (walker == NULL){
        // If it walks through the entire list and can't find 'data', 
        // then it doesn't exists, so return NULL
        printf("ERROR: can't find data.");
        pthread_rwlock_unlock(&rw_lock);
        return NULL;
    }
    else{
        pthread_rwlock_unlock(&rw_lock);
        return walker;
    }
}

// To display the whole list, just call list_display_range() with the range of the whole list
void list_display(Node** head){
    return list_display_range(head, NULL, NULL);
}

// Displays the entire list in format [0, 1, 2, 3, etc]
void list_display_range(Node** head, Node* start_node, Node* end_node){
    pthread_rwlock_rdlock(&rw_lock);

    // By default, traversing starts at head, 
    // but if 'start node' is specified, start there instead
    Node* walker = *head;
    if(start_node != NULL)
        walker = start_node;
        
    printf("[");
    printf("%d", walker->data);
    // Walk through list and print until we reach either 'end_node' or NULL
    while(walker->next != NULL && walker != end_node){
        walker = walker->next;
        printf(", %d", walker->data);
    }
    printf("]");

    pthread_rwlock_unlock(&rw_lock);
}

// Counts amount of nodes in list by traversing through the whole list and increasing count for each one
int list_count_nodes(Node** head){
    pthread_rwlock_rdlock(&rw_lock);

    int count = 0;
    Node* walker = *head;
    while(walker != NULL){
        count++;
        walker = walker->next;
    }

    pthread_rwlock_unlock(&rw_lock);
    return count;
}

// Deinitializes the list, by freeing all related memory
void list_cleanup(Node** head){
    pthread_rwlock_wrlock(&rw_lock);
    Node* walker = *head;
    
    // This is unneccessary
    while(walker != NULL){
        Node* toDel = walker;
        walker = walker->next;

        mem_free(toDel);
    }

    // Deinitializes the memory
    mem_deinit();
    *head = NULL;

    pthread_rwlock_unlock(&rw_lock);
    //pthread_rwlock_destroy(&lock);
}
