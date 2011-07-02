#ifndef __LOGGER__
#define __LOGGER__
#ifndef TRACE
#define TRACE 1
#endif
#if 0
#define LOG(_hdl, _msg)                                 \
{                                                       \
        if(TRACE > 0)                                   \
        fprintf(_hdl,"Pid=%d:%s %s",getpid(),__FUNCTION__, _msg);       \
}
#endif
#define LOGK(_msg)					\
{							\
	if(TRACE > 0)					\
		printk("%s",_msg);			\
}
#endif
