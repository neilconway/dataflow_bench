/*
 * Simple dataflow benchmark; requires APR. Compile with:
 * 
 *     gcc -fno-strict-aliasing -O2 -Wall `apr-1-config --cppflags --includes --link-ld --libs` bench.c -o bench
 */
#include <apr_general.h>
#include <apr_hash.h>
#include <apr_pools.h>
#include <apr_time.h>
#include <stdio.h>

struct Op;

typedef void (*op_invoke_func)(struct Op *op, int *t);

struct Op
{
    struct Op *next;
    op_invoke_func invoke;
};

struct PredicateOp
{
    struct Op op;
    int pred;
};

struct JoinOp
{
    struct Op op;
    apr_hash_t *tbl;
};

struct ListNode
{
    struct ListNode *next;
    int *t;
};

struct SinkOp
{
    struct Op op;
    apr_pool_t *pool;
    struct ListNode *list_head;
};

static void pred_op_invoke(struct Op *op, int *t);
static void join_op_invoke(struct Op *op, int *t);
static void sink_op_invoke(struct Op *op, int *t);

static struct Op *
pred_op_init(apr_pool_t *p, struct Op *next, int pred)
{
    struct PredicateOp *result;

    result = apr_palloc(p, sizeof(*result));
    result->op.next = next;
    result->op.invoke = pred_op_invoke;
    result->pred = pred;

    return (struct Op *) result;
}

static void
pred_op_invoke(struct Op *op, int *t)
{
    struct PredicateOp *self = (struct PredicateOp *) op;

    if (self->pred != *t)
    {
        struct Op *next = self->op.next;
        next->invoke(next, t);
    }
}

static struct Op *
join_op_init(apr_pool_t *p, struct Op *next)
{
    struct JoinOp *result;
    int i;

    result = apr_palloc(p, sizeof(*result));
    result->op.next = next;
    result->op.invoke = join_op_invoke;
    result->tbl = apr_hash_make(p);

    for (i = 0; i < 12000; i++)
    {
        int *iptr;

        if (i % 2 != 0)
            continue;

        iptr = apr_palloc(p, sizeof(*iptr));
        *iptr = i;
        apr_hash_set(result->tbl, iptr, sizeof(*iptr), iptr);
    }

    return (struct Op *) result;
}

static void
join_op_invoke(struct Op *op, int *t)
{
    struct JoinOp *self = (struct JoinOp *) op;

    if (apr_hash_get(self->tbl, t, sizeof(*t)) == NULL)
    {
        struct Op *next = self->op.next;
        next->invoke(next, t);
    }
}

static struct Op *
sink_op_init(apr_pool_t *p)
{
    struct SinkOp *result;

    result = apr_palloc(p, sizeof(*result));
    result->op.next = NULL;
    result->op.invoke = sink_op_invoke;
    result->pool = p;
    result->list_head = NULL;

    return (struct Op *) result;
}

static void
sink_op_invoke(struct Op *op, int *t)
{
    struct SinkOp *self = (struct SinkOp *) op;
    struct ListNode *n;

    n = apr_palloc(self->pool, sizeof(*n));
    n->next = self->list_head;
    n->t = t;
    self->list_head = n;
}

static int *
intdup(apr_pool_t *p, int i)
{
    int *copy;

    copy = apr_palloc(p, sizeof(*copy));
    *copy = i;
    return copy;
}

int
main(void)
{
    int i;

    (void) apr_initialize();

    for (i = 0; i < 10; i++)
    {
        apr_pool_t *pool;
        struct Op *op;
        int j;
        apr_time_t start_time;

        start_time = apr_time_now();
        (void) apr_pool_create(&pool, NULL);

        op = sink_op_init(pool);
        op = pred_op_init(pool, op, 4);
        op = pred_op_init(pool, op, 6);
        op = join_op_init(pool, op);
        op = pred_op_init(pool, op, 8);
        op = pred_op_init(pool, op, 10);
        op = join_op_init(pool, op);

        for (j = 0; j < 2000000; j++)
        {
            if (j % 2 == 1)
                op->invoke(op, intdup(pool, j));
        }

        (void) apr_pool_destroy(pool);
        printf("Duration: %" APR_TIME_T_FMT " usec\n",
               (apr_time_now() - start_time));
    }

    apr_terminate();
    return 0;
}
