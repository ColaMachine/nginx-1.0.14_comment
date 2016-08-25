
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <nginx.h>


ngx_int_t   ngx_ncpu;
ngx_int_t   ngx_max_sockets;
ngx_uint_t  ngx_inherited_nonblocking;
ngx_uint_t  ngx_tcp_nodelay_and_tcp_nopush;


struct rlimit  rlmt;


ngx_os_io_t ngx_os_io = {
    ngx_unix_recv,
    ngx_readv_chain,
    ngx_udp_unix_recv,
    ngx_unix_send,
    ngx_writev_chain,
    0
};


ngx_int_t
ngx_os_init(ngx_log_t *log)
{
    ngx_uint_t  n;

#if (NGX_HAVE_OS_SPECIFIC_INIT)
    if (ngx_os_specific_init(log) != NGX_OK) {//zheli ngx_linux_init.c 35 ngx_os_io_t = ngx_linux_io //:tabnew os/unix/ngx_linux_init.c
        return NGX_ERROR;
    }
#endif
//http://www.xuebuyuan.com/2226475.html
    ngx_init_setproctitle(log);//为修改进程名称做准备at src/os/unix/ngx_setproctitle.c:36 ba suo you huan jing bian liang dou fangru dao dui zhan zhong 

    ngx_pagesize = getpagesize();//获取页大小4096
    ngx_cacheline_size = NGX_CPU_CACHE_LINE;//64

    for (n = ngx_pagesize; n >>= 1; ngx_pagesize_shift++) { /* void */ }//ngx_pagesize_shift的值为log2 ngx_pagesize

#if (NGX_HAVE_SC_NPROCESSORS_ONLN)
    if (ngx_ncpu == 0) {//zhixing zheli    sysconf() 返回选项 (变量) 的当前值
        ngx_ncpu = sysconf(_SC_NPROCESSORS_ONLN);//../sysdeps/unix/sysv/linux/x86_64/sysconf.c
    }//ngx_ncpu =4 
#endif

    if (ngx_ncpu < 1) {//4
        ngx_ncpu = 1;// bu zhi xing
    }

    ngx_cpuinfo();// at src/core/ngx_cpuinfo.c:74
    //RLIMIT_NOFILE 限制指定了进程可以打开的最多文件数。
    if (getrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
        ngx_log_error(NGX_LOG_ALERT, log, errno,
                      "getrlimit(RLIMIT_NOFILE) failed)");
        return NGX_ERROR;
    }

    ngx_max_sockets = (ngx_int_t) rlmt.rlim_cur;//1024

#if (NGX_HAVE_INHERITED_NONBLOCK || NGX_HAVE_ACCEPT4)
    ngx_inherited_nonblocking = 1;
#else
    ngx_inherited_nonblocking = 0;
#endif

    srandom(ngx_time());//初始化随机数

    return NGX_OK;
}


void
ngx_os_status(ngx_log_t *log)
{
    ngx_log_error(NGX_LOG_NOTICE, log, 0, NGINX_VER);

#ifdef NGX_COMPILER
    ngx_log_error(NGX_LOG_NOTICE, log, 0, "built by " NGX_COMPILER);
#endif

#if (NGX_HAVE_OS_SPECIFIC_INIT)
    ngx_os_specific_status(log);
#endif

    ngx_log_error(NGX_LOG_NOTICE, log, 0,
                  "getrlimit(RLIMIT_NOFILE): %r:%r",
                  rlmt.rlim_cur, rlmt.rlim_max);
}


ngx_int_t
ngx_posix_post_conf_init(ngx_log_t *log)
{
    ngx_fd_t  pp[2];

    if (pipe(pp) == -1) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "pipe() failed");
        return NGX_ERROR;
    }

    if (dup2(pp[1], STDERR_FILENO) == -1) {
        ngx_log_error(NGX_LOG_EMERG, log, errno, "dup2(STDERR) failed");
        return NGX_ERROR;
    }

    if (pp[1] > STDERR_FILENO) {
        if (close(pp[1]) == -1) {
            ngx_log_error(NGX_LOG_EMERG, log, errno, "close() failed");
            return NGX_ERROR;
        }
    }

    return NGX_OK;
}
