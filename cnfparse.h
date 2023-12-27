#pragma once

struct systemcnf_s {
	char *boot2;
	char *ver;
	char *vmode;
	char *hddunitpower;
};

extern struct systemcnf_s parse_systemcnf(void *buf, size_t siz);
extern void free_systemcnf(struct systemcnf_s cnf);
