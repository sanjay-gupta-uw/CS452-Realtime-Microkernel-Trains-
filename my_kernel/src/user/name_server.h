#ifndef _NAME_SERVER_H_
#define _NAME_SERVER_H_

void NameServer();
int REGISTERAS(const char* name);
int WHOIS(const char* name);
void setNameServerTid(int tid);

#endif // _NAME_SERVER_H_
