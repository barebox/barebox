#ifndef __CACHE_H
#define __CACHE_H

int	icache_status (void);
void	icache_enable (void);
void	icache_disable(void);
int	dcache_status (void);
void	dcache_enable (void);
void	dcache_disable(void);
int	checkicache   (void);
int	checkdcache   (void);

#endif /* __CACHE_H */

