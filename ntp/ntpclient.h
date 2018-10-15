/* NTP client
 *
 * Copyright (C) 1997-2015  Larry Doolittle <larry@doolittle.boa.org>
 * Copyright (C) 2010-2018  Joachim Nilsson <troglobit@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (Version 2,
 * June 1991) as published by the Free Software Foundation.  At the
 * time of writing, that license was published by the FSF with the URL
 * http://www.gnu.org/copyleft/gpl.html, and is incorporated herein by
 * reference.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef NTPCLIENT_H_
#define NTPCLIENT_H_

#include <stdarg.h>		/* vsyslog(), vfprintf(), use -D_BSD_SOURCE */
#ifdef ENABLE_SYSLOG
# include <syslog.h>
# include <sys/utsname.h>
# include <sys/time.h>
# define LOG_OPTS        (LOG_NOWAIT | LOG_PID)
# define LOG_FACILITY    LOG_CRON
# define LOG_OPTION      "L"
#else
# define LOG_ERR         3
# define LOG_WARNING     4
# define LOG_NOTICE      5
# define LOG_INFO        6
# define LOG_DEBUG       7
# define LOG_OPTION
#endif


#include <stdint.h>

extern int debug;
extern int log_enable;
extern int verbose;

extern double min_delay;        /* global tuning parameter */

/* prototypes for functions defined in ntpclient.c */
void logit(int severity, int syserr, const char *format, ...);

/* prototype for function defined in phaselock.c */
int contemplate_data(unsigned int absolute, double skew, double errorbar, int freq);

struct ntp_control {
    uint32_t time_of_send[2];
    int usermode;
    int live;
    int set_clock;		/* non-zero presumably needs root privs */
    int probe_count;
    int cycle_time;
    int goodness;
    int cross_check;

    uint16_t local_udp_port;
    char *server;		/* must be set */
    char serv_addr[4];
};

void primary_loop(int usd, struct ntp_control *ntpc);
int setup_socket(struct ntp_control *ntpc);
void setup_signals(void);
void send_packet(int usd, uint32_t time_sent[2]);
int getaddrbyname(char *host, struct sockaddr_storage *ss);
#endif /* NTPCLIENT_H_ */
