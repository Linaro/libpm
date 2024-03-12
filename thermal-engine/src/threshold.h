#ifndef __THRESHOLD_H__
#define __THRESHOLD_H__

#define THRESHOLD_DEFAULT_TEMP 21000

struct thresholds;
struct plugin_power;
struct list;

int threshold_crossed_up(struct thresholds *thresholds, int tz_id, int temperature);
int threshold_crossed_down(struct thresholds *thresholds, int tz_id, int temperature);
int threshold_add(struct thresholds *thresholds, int tz_id, int temperature);
int threshold_add_action(struct thresholds *thresholds, struct list *plugins,
			 struct plugin_power *pwr, const char *profile,
			 int tz_id, int temperature);
#endif
