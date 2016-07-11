#ifndef _STACK_H
#define _STACK_H

struct stack
{
    struct node;

    inline node* pop()
    {
        node* n = head.next;
        if (n == NULL) return NULL;
        head.next = n->next;
        return n;
    }

    inline void push(void* block)
    {
        node* n = reinterpret_cast<node*>(block);
        n->next = head.next;
        head.next = n;
    }

    inline void clear()
    {
        head.next = NULL;
    }

    struct node
    {
        node()
        {
            next = NULL;
        }
        node* next;
    };

private:
    node head;
};

#endif
