/* QUESTIONS AT EACH STEP
 * 1. is the pid new or does it exist
 * 2. does the frame table contain the page requested
 */

/* At the end what do we need to report:
 * 1. <PID> <# PAGE FAULTS> (For each PID in order of arrival) 
 * 2. <PID> <PROCESS TABLE> (For each PID in order of arrival)
 *          <PAGE ID> <FRAME ID> (For each PAGE held by PID)
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

typedef uint32_t u32;
typedef uint16_t std_id_width;
typedef std_id_width processid_t;
typedef std_id_width pageid_t;
typedef struct frame
{
    bool used;
    bool tick;
    pageid_t page;
    processid_t pid;
    struct frame *left;
    struct frame *right;

} frame_t;
typedef struct framelist
{
    size_t size;
    frame_t *head;
    frame_t *tail;
    frame_t *cursor;

} framelist_t;
typedef struct process
{
    processid_t id;
    u32 faults;
    size_t num_pages;
    size_t num_frames;
    framelist_t *frames;
    struct process *right;
    struct process *left;

} process_t;
typedef struct process_list
{
    size_t size;
    process_t *head;
    process_t *tail;
}plist_t;

frame_t *init_frame(pageid_t, processid_t);
framelist_t *init_framelist();
void add_frame(process_t*, frame_t*);
framelist_t *make_framelist(size_t);
void free_framelist(framelist_t*);
process_t *init_process(processid_t);
process_t* add_process(plist_t*, processid_t);
bool has_process(plist_t *plist, processid_t pid);
process_t *find_process(plist_t *plist, processid_t pid);
plist_t *init_plist();
void print_plist(plist_t *plist);
bool iscached(process_t*, pageid_t);
void global_replacement(process_t*, pageid_t);
void local_replacement(process_t*, pageid_t);

#define BSIZE 8
#define checkin(var) printf("[ ? ] - " #var " = %u\n", var);

frame_t *find_frame(plist_t *plist, pageid_t pageid);
frame_t *find_frame(plist_t *plist, pageid_t pageid)
{
    process_t *p = plist->head;
    frame_t *f;
    bool frame_found = false;
    size_t count = plist->size;

    while(p && !frame_found && count--)
    {
        f = p->frames->head;
        for (size_t i = 0; i < p->frames->size; i++)
        {
         if(f->page == pageid)
         {
            printf("page %u was found in frame %u of process %u\n", pageid, i, p->id);
            frame_found = true;
            break;
         }
         else
             f = f->right;
        }
        p = p->right;
        if(!(p))
            break;
    }
    return frame_found ? f : NULL;
}

int main()
{
    // decide if global or local store in state
    bool is_global;
    char buff[BSIZE];
    {
        /* read first line */
        assert(fgets(buff, BSIZE, stdin));
        char *c = &buff[0];
        assert(*c == 'L' || *c == 'G');
        is_global = *c == 'G';
    }
    size_t num_frames = 0;
    {
        assert(fgets(buff, BSIZE, stdin));
        sscanf(buff, "%u", &num_frames);
        assert(num_frames);
    }

    u32 pid, page;
    plist_t *plist = init_plist();
    framelist_t *global_frames;
    void (*replace)(process_t *, pageid_t);

    if(is_global)
    {
        replace = &global_replacement;
        global_frames = init_framelist();
    }
    else
    {
        replace = &local_replacement;
        global_frames = NULL;
    }

    uint32_t count = 0;

    size_t line_width;
    while((fgets(buff,BSIZE,stdin)) && (line_width = strlen(buff)) > 1)
    {
        if(buff[0] == '\n')
            break;

        page = pid = 0;
        sscanf(buff, "%u %u", &pid, &page);

        assert(pid);
        assert(page);

        process_t *p;
        if(has_process(plist, pid))
        {
            p = find_process(plist, pid);
            assert(p);
        }
        else
        {
            p = add_process(plist, pid);
            p->num_frames = num_frames;
            p->frames = is_global?global_frames:init_framelist();
        }
        assert(p->frames);
        if(!iscached(p, page))
        {
            if (p->num_frames > p->frames->size)
                add_frame(p,init_frame(page, p->id));
            else (*replace)(p, page);
        }
        else
            ;
    }

    process_t *p = plist->head;

    printf("PID Page Faults\n");
    count = plist->size;
    while(p && count--)
    {
        printf("%u\t%u\n", p->id, p->faults);
        p = p->right;
    }
  
    p = plist->head;
    u32 frame_count = 0;
    count = plist->size;
    while(p && count--)
    {
        if(!is_global)
            frame_count = 0;
        printf("Process %u page table\nPage Frame\n", p->id);
        frame_t *f = p->frames->head;
        size_t cc = p->frames->size;
        while(cc--)
        {
            if(f->pid == p->id)
            { 
                printf("%u\t%u\n", f->page, frame_count++);
            }
            f = f->right;
        }
        p = p->right;
    }

    /* 
     * FREE MEMORY
     */
    return 0;
}

frame_t *init_frame(pageid_t page, processid_t pid)
{
    frame_t *frame;
    frame = malloc(sizeof(frame_t));
    assert(frame);
    frame->used = false;
    frame->tick = false;
    frame->page = page;
    frame->pid = pid;
    return frame;
}

framelist_t *init_framelist()
{
    framelist_t *framelist;
    if (!(framelist = malloc(sizeof(framelist_t))))
        return NULL;
    else
        framelist->size = 0;

    return framelist;
}

void add_frame(process_t *p, frame_t *frame)
{
    assert(p->frames);
    if(p->frames->head == NULL)
    {
        p->frames->head = frame;
        p->frames->tail = p->frames->head;
        p->frames->head->right = p->frames->tail;
        p->frames->tail->left = p->frames->head;
        p->frames->cursor = p->frames->head;
    } 
    else
    {
        frame->right = p->frames->tail;
        p->frames->tail->left = frame;
        frame->left = p->frames->head;
        p->frames->tail = frame;
    }
    p->frames->size += 1;
    assert(p->frames->size <= p->num_frames);
    p->faults += 1;
}


void free_framelist(framelist_t *framelist)
{
    while(framelist && framelist->head)
    {
        frame_t *temp = framelist->head;
        *framelist->head = *framelist->head->right;
        free(temp);
    }
}

process_t *init_process(processid_t pid)
{
    process_t *p = malloc(sizeof(process_t));
    assert(p);
    p->id = pid;
    p->faults = 0;
    p->num_pages = 0;
    p->num_frames = 0;
    p->left = NULL;
    p->right = NULL;
    return p;
}

process_t * find_process(plist_t *plist, processid_t pid)
{
    process_t *p = plist->head;
    size_t count = plist->size;
    while(p && count--)
    {
        if(p->id == pid)
            return p;
        p = p->right;
    }
    return NULL;
}
bool has_process(plist_t *plist, processid_t pid)
{
    size_t count = plist->size;
    process_t *ptr = plist->head;
    while(ptr && count--)
    {
       if(ptr->id == pid)
           return true;
       ptr = ptr->right;
    }
    return false;
}
plist_t *init_plist()
{
    plist_t *plist;
    assert((plist = malloc(sizeof(plist_t))));
    return plist;
}

void print_plist(plist_t *plist)
{
    process_t *p = plist->head;
    if(plist->head)
    {
        size_t count = plist->size;
        checkin(count);
        while (count--)
        {
            p = p->right;
        }
    }
    else
    {
    }
}

bool id_in_plist(plist_t *plist, processid_t pid)
{
    process_t *ptr = plist->head;
    size_t count = plist->size;
    while (ptr!=NULL && count-- )
    {
        if (ptr->id == pid)
            return true;
        ptr = ptr->right;
    }
    return false;
}


process_t *add_process(plist_t *plist, processid_t pid)
{
    if(plist->head == NULL)
    {
        plist->head = init_process(pid);
        plist->head->right = plist->head;
        plist->head->left = plist->head;
        plist->tail = plist->head;
        plist->size++;
        return plist->tail;
    }
    else
    {
        process_t *p = init_process(pid);
        p->left = plist->tail;
        plist->tail->right = p;
        p->right = plist->head;
        plist->tail = p;
        plist->size += 1;
        return plist->tail;
    }

    return NULL;
}

bool iscached(process_t * process, pageid_t virtualpageid)
{
    frame_t *f = process->frames->head;
    for (size_t i=0; i < process->frames->size; i++) // TODO FIX ME
    {
        if(f->page == virtualpageid && f->pid == process->id)
            return true;
        else
            f = f->right;
    }
    return false;
}

void global_replacement(process_t *p, pageid_t page)
{
    assert(p->frames->size);
    frame_t *ft = p->frames->head;
    p->frames->head = p->frames->head->right;
    p->frames->head->left = p->frames->tail;
    free(ft);
    p->frames->size -= 1;
    add_frame(p, init_frame(page, p->id));
}

void local_replacement(process_t *p, pageid_t page)
{
    
    while(!p->frames->cursor->tick)
    {
        p->frames->cursor->tick = true;
        p->frames->cursor = p->frames->cursor->right;
    }

    p->frames->cursor->page = page;
    p->frames->cursor->pid = p->id;
    p->faults++;
}
