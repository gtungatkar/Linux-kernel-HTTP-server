/* 
 * sxg's debug macro
 * 
 * @2007-2008, China
 * @song.xian-guang@hotmail.com (MSN Accounts)
 * 
 * This code is licenced under the GPL
 * Feel free to contact me if any questions
 * 
 */
#ifndef _SXG_DEBUG_H_
#define _SXG_DEBUG_H_

/* debug macro*/
#ifdef  _sxg_debug_
#	define sxg_debug(fmt, args...)	printk("ksocket : %s, %s, %d, "fmt, __FILE__, __FUNCTION__, __LINE__, ##args)
#else
#	define sxg_debug(fmt, args...)
#endif

#endif /* !_SXG_DEBUG_H_ */
