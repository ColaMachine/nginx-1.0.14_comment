
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


#if (NGX_SETPROCTITLE_USES_ENV)

/*
 * To change the process title in Linux and Solaris we have to set argv[1]
 * to NULL and to copy the title to the same place where the argv[0] points to.
 * However, argv[0] may be too small to hold a new title.  Fortunately, Linux
 * and Solaris store argv[] and environ[] one after another.  So we should
 * ensure that is the continuous memory and then we allocate the new memory
 * for environ[] and copy it.  After this we could use the memory starting
 * from argv[0] for our process title.
 *
 * The Solaris's standard /bin/ps does not show the changed process title.
 * You have to use "/usr/ucb/ps -w" instead.  Besides, the UCB ps dos not
 * show a new title if its length less than the origin command line length.
 * To avoid it we append to a new title the origin command line in the
 * parenthesis.
 */

extern char **environ;

static char *ngx_os_argv_last;

ngx_int_t
ngx_init_setproctitle(ngx_log_t *log)//chushi hua she zhi ng 初始化nginx设置进程标题
{
    u_char      *p;
    size_t       size;
    ngx_uint_t   i;

    size = 0;

    for (i = 0; environ[i]; i++) {//LC_PAPER=zh_CN.UTF-8 p environ[0]
        size += ngx_strlen(environ[i]) + 1;//计算总长度
    }//大概有40多个

    p = ngx_alloc(size, log);//分配一个总长度内存
    if (p == NULL) {
        return NGX_ERROR;
    }
    //0x7fffffffe791
    ngx_os_argv_last = ngx_os_argv[0];///home/colamachine/git/nginx-1.0.14_comment/objs/nginx//原始main函数的代入参数argv
//  ngx_os_argv[0] fffffffe791 [1]0x7fffffffe7c7 [2]0x7fffffffe7ca+58 =+0x3a= 804+1 =805   0x7fffffffe805  p strlen(ngx_os_argv[2])=58 =0x 48+10= 0x 3a
    for (i = 0; ngx_os_argv[i]; i++) {//这个 ngx_os_argv 是 运行 命令  nginx -c nginx.conf类似的
        if (ngx_os_argv_last == ngx_os_argv[i]) {//0 0x7fffffffe791 0x7fffffffe7c7  0x7fffffffe7ca
            ngx_os_argv_last = ngx_os_argv[i] + ngx_strlen(ngx_os_argv[i]) + 1;//指向字符串最后位置  让ngx_os_argv_last一直指向到最后一个ngx_os_argv的末尾/0的位置
        }
    }//这个argv[]与environ两个变量所占的内存是连续的，并且是environ紧跟在argv[]后面。

    for (i = 0; environ[i]; i++) {
        if (ngx_os_argv_last == environ[i]) {//???? 2   environ[0] 0x7fffffffe805

            size = ngx_strlen(environ[i]) + 1;//留一个/0 21
            ngx_os_argv_last = environ[i] + size; //21

            ngx_cpystrn(p, (u_char *) environ[i], size);
            environ[i] = (char *) p;//将environ的内容全部转存到堆中
            p += size;//p往后移动准备接收下一个字符串
        }
    }

    ngx_os_argv_last--;//指向最后一个environ 的最后的值

    return NGX_OK;
}


void
ngx_setproctitle(char *title)
{
    u_char     *p;

#if (NGX_SOLARIS)

    ngx_int_t   i;
    size_t      size;

#endif

    ngx_os_argv[1] = NULL;

    p = ngx_cpystrn((u_char *) ngx_os_argv[0], (u_char *) "nginx: ",
                    ngx_os_argv_last - ngx_os_argv[0]);

    p = ngx_cpystrn(p, (u_char *) title, ngx_os_argv_last - (char *) p);

#if (NGX_SOLARIS)

    size = 0;

    for (i = 0; i < ngx_argc; i++) {
        size += ngx_strlen(ngx_argv[i]) + 1;
    }

    if (size > (size_t) ((char *) p - ngx_os_argv[0])) {

        /*
         * ngx_setproctitle() is too rare operation so we use
         * the non-optimized copies
         */

        p = ngx_cpystrn(p, (u_char *) " (", ngx_os_argv_last - (char *) p);

        for (i = 0; i < ngx_argc; i++) {
            p = ngx_cpystrn(p, (u_char *) ngx_argv[i],
                            ngx_os_argv_last - (char *) p);
            p = ngx_cpystrn(p, (u_char *) " ", ngx_os_argv_last - (char *) p);
        }

        if (*(p - 1) == ' ') {
            *(p - 1) = ')';
        }
    }

#endif

    if (ngx_os_argv_last - (char *) p) {
        ngx_memset(p, NGX_SETPROCTITLE_PAD, ngx_os_argv_last - (char *) p);
    }

    ngx_log_debug1(NGX_LOG_DEBUG_CORE, ngx_cycle->log, 0,
                   "setproctitle: \"%s\"", ngx_os_argv[0]);
}

#endif /* NGX_SETPROCTITLE_USES_ENV */
