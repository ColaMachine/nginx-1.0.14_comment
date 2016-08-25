
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


/*
 * The strerror() messages are copied because:
 *
 * 1) strerror() and strerror_r() functions are not Async-Signal-Safe,
 *    therefore, they cannot be used in signal handlers;
 *
 * 2) a direct sys_errlist[] array may be used instead of these functions,
 *    but Linux linker warns about its usage:
 *
 * warning: `sys_errlist' is deprecated; use `strerror' or `strerror_r' instead
 * warning: `sys_nerr' is deprecated; use `strerror' or `strerror_r' instead
 *
 *    causing false bug reports.
 */


static ngx_str_t  *ngx_sys_errlist;
static ngx_str_t   ngx_unknown_error = ngx_string("Unknown error");


u_char *
ngx_strerror(ngx_err_t err, u_char *errstr, size_t size)// 
{
    ngx_str_t  *msg;

    msg = ((ngx_uint_t) err < NGX_SYS_NERR) ? &ngx_sys_errlist[err]:
                                              &ngx_unknown_error;//if the err is between 0~135 return the arry  value
    size = ngx_min(size, msg->len);//get the min value 

    return ngx_cpymem(errstr, msg->data, size);
}

//ready the ngx_sys_errlist
ngx_uint_t
ngx_strerror_init(void)
{
    char       *msg;//pointer to char 
    u_char     *p;//u_char pointer
    size_t      len;//应为unsigned int，在64位系统中为 long unsigned int。
    ngx_err_t   err;//ngx_errno.h typedef int 

    /*
     * ngx_strerror() is not ready to work at this stage, therefore,
     * malloc() is used and possible errors are logged using strerror().
     */

    len = NGX_SYS_NERR * sizeof(ngx_str_t);// len= 2160准备分配NGX_SYS_NERR*16个大小的空间 NGX_SYS_ERR defined in ngx_auto_config.h 135 ngx_str_t size-t=8 u_char *data=8
   
    ngx_sys_errlist = malloc(len);//malloc(16*135)
    if (ngx_sys_errlist == NULL) {//if malloc fail
        goto failed;//goto row 81
    }

    for (err = 0; err < NGX_SYS_NERR; err++) {//遍历系统的所有错误代码
        msg = strerror(err);//strerror(0..134)//得到系统对应的错误消息字符串
        len = ngx_strlen(msg);//get the len strlen 8 算出错误消息的字符长度

        p = malloc(len);//malloc mem for str 8 在堆中分配指定
        if (p == NULL) {
            goto failed;
        }

        ngx_memcpy(p, msg, len);//copy the error string to p 
        ngx_sys_errlist[err].len = len;//==len
        ngx_sys_errlist[err].data = p;// p is the mem copy from strerror(1..134)
    }

    return NGX_OK;//the right way

failed://the fail way

    err = errno;//get the errno
    ngx_log_stderr(0, "malloc(%uz) failed (%d: %s)", len, err, strerror(err));//print the error str

    return NGX_ERROR;
}
