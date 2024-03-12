#ifndef __PROFILE_H__
#define __PROFILE_H__

const char *profile_get_name(struct profile *profile);

int profile_set_name(struct profile *profile, const char *name);

#endif /* __PROFILE_H__ */
