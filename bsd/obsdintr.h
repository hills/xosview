#ifndef _OPENBSDINTR_H_
#define _OPENBSDINTR_H_

/* XXX: gcc defines i386, but egcs defines __i386__ */
#if defined(i386) && !defined(__i386__)
#define __i386__
#endif

/* XXX: gcc defines pc532, but egcs defines __pc532__ */
#if defined(pc532) && !defined(__pc532__)
#define __pc532__
#endif

#ifdef __i386__
struct intrhand {
	int	(*ih_fun) __P((void *));
	void	*ih_arg;
	u_long	ih_count;
	struct	intrhand *ih_next;
	int	ih_level;
	int	ih_irq;
	char	*ih_what;
};
#endif

#ifdef __pc532__
struct iv {
	void (*iv_vec)();
	void *iv_arg;
	int iv_cnt;
	char *iv_use;
};
#endif

#endif /* _OPENBSDINTR_H_ */
