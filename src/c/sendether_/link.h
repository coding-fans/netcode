/*
 * Author: fasion
 * Created time: 2021-01-13 15:45:17
 * Last Modified by: fasion
 * Last Modified time: 2021-02-18 15:21:31
 */

int mac_aton(const char *a, unsigned char *n);
int fetch_iface_mac(int s, const char *iface, unsigned char *mac);
int fetch_iface_index(int s, const char *iface);
