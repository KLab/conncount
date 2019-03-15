#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<getopt.h>
#include<sys/socket.h>
#include<netinet/tcp.h>
#include<linux/netlink.h>
#include<linux/inet_diag.h>

#define TCP_STATE_MASK  0xF
#define TCP_ACTION_FIN  (1 << 7)

enum {
  TCPF_ESTABLISHED = (1 << 1),
  TCPF_SYN_SENT    = (1 << 2),
  TCPF_SYN_RECV    = (1 << 3),
  TCPF_FIN_WAIT1   = (1 << 4),
  TCPF_FIN_WAIT2   = (1 << 5),
  TCPF_TIME_WAIT   = (1 << 6),
  TCPF_CLOSE       = (1 << 7),
  TCPF_CLOSE_WAIT  = (1 << 8),
  TCPF_LAST_ACK    = (1 << 9),
  TCPF_LISTEN      = (1 << 10),
  TCPF_CLOSING     = (1 << 11)
};

static const char *tcp_state[] = {
    "", /* 0 */
    "ESTABLISHED",
    "SYN_SENT",
    "SYN_RECV",
    "FIN_WAIT1",
    "FIN_WAIT2",
    "TIME_WAIT",
    "CLOSED",
    "CLOSE_WAIT",
    "LAST_ACK",
    "LISTEN",
    "CLOSING"
};

static const int opt_state[] = {
  0,
  TCPF_ESTABLISHED,
  TCPF_SYN_SENT,
  TCPF_SYN_RECV,
  TCPF_FIN_WAIT1,
  TCPF_FIN_WAIT2,
  TCPF_TIME_WAIT,
  TCPF_CLOSE,
  TCPF_CLOSE_WAIT,
  TCPF_LAST_ACK,
  TCPF_LISTEN,
  TCPF_CLOSING
};

int count[sizeof tcp_state/sizeof tcp_state[0]];

void usage()
{
  printf("usage: conncount [OPTION] port\n");
  printf("  -l, --listen\n");
  printf("  -e, --established\n");
  printf("  -t, --time-wait\n");
  printf("  -c, --close\n");
  printf("  -f, --fin\n");
  printf("  -s, --syn\n");
  printf("  -v, --verbose\n");
  exit(0);
}

int showcount(int s)
{
  int i;

  for (i = 1; i < sizeof count/sizeof count[0]; i++) {
    if (s & opt_state[i]) {
      printf("%s %d\n", tcp_state[i], count[i]);
    }
  }
}

int scount(int n, int p, int s)
{
  int r=0;
  int len;
  static int  seq=1;
  static char rbuf[65535];
  struct {
    struct nlmsghdr      nlh;
    struct inet_diag_req req;
  } wbuf;
  struct nlmsghdr       *nlh;
  struct inet_diag_msg  *msg;

  memset(count, 0, sizeof count);
  wbuf.nlh.nlmsg_len          = NLMSG_ALIGN(NLMSG_LENGTH(sizeof(wbuf.req)));
  wbuf.nlh.nlmsg_type         = TCPDIAG_GETSOCK;
  wbuf.nlh.nlmsg_flags        = NLM_F_REQUEST | NLM_F_DUMP;
  wbuf.nlh.nlmsg_seq          = seq++;
  wbuf.req.idiag_family       = AF_INET;
  wbuf.req.idiag_src_len      = 0;
  wbuf.req.idiag_dst_len      = 0;
  wbuf.req.idiag_ext          = 0; 
  wbuf.req.idiag_states       = s;
  wbuf.req.idiag_dbs          = 0;
  wbuf.req.id.idiag_sport     = htons(p);
  wbuf.req.id.idiag_dport     = 0;
  wbuf.req.id.idiag_cookie[0] = INET_DIAG_NOCOOKIE;
  wbuf.req.id.idiag_cookie[1] = INET_DIAG_NOCOOKIE;
  len = write(n, &wbuf, wbuf.nlh.nlmsg_len);
  if(len < 0){
    fprintf(stderr,"netlink write error!!\n");
    return(-1);
  }
  while(1){
    len=read(n, rbuf, sizeof(rbuf));
    if(len == -1){
      fprintf(stderr,"netlink read error!!\n");
      return(-1);
    }
    for(nlh=(struct nlmsghdr *)rbuf;NLMSG_OK(nlh, len);nlh=NLMSG_NEXT(nlh, len)){
      if(nlh->nlmsg_seq != wbuf.nlh.nlmsg_seq)
        continue;
      msg=(struct inet_diag_msg *)((char *)nlh + sizeof(struct nlmsghdr));
      if(nlh->nlmsg_type == NLMSG_ERROR){
        fprintf(stderr,"netlink msg error\n");
        return(-1);
      }
      if(nlh->nlmsg_type == NLMSG_DONE){
        return(r);
      }
      if(msg->idiag_family != AF_INET){
        continue;
      }
      count[msg->idiag_state]++;
      r++;
    }
  }
  return(0);
}

int main(int argc, char *argv[])
{
  int r;
  int c;
  int p;
  int n;
  int s;
  int v;

  struct option opt[]={
    {"help",        0, NULL, 'h'},
    {"listen",      0, NULL, 'l'},
    {"established", 0, NULL, 'e'},
    {"timewait",    0, NULL, 't'},
    {"close",       0, NULL, 'c'},
    {"fin",         0, NULL, 'f'},
    {"syn",         0, NULL, 's'},
    {"verbose",     0, NULL, 'v'},
    {0, 0, 0, 0}
  };

  s = 0;
  v = 0;
  while((r=getopt_long(argc, argv, "hletcfsv", opt, NULL)) != -1){
    switch(r){
      case 'e':
        s |= TCPF_ESTABLISHED;
        break;

      case 'l':
        s |= TCPF_LISTEN;
        break;

      case 't':
        s |= TCPF_TIME_WAIT;
        break;

      case 'c':
        s |= TCPF_CLOSE;
        s |= TCPF_CLOSE_WAIT;
        s |= TCPF_CLOSING;
        break;

      case 'f':
        s |= TCPF_FIN_WAIT1;
        s |= TCPF_FIN_WAIT2;
        break;

      case 's':
        s |= TCPF_SYN_SENT;
        s |= TCPF_SYN_RECV;
        break;

      case 'v':
        v = 1;
        break;

      case 'h':
        usage();

      case '?':
        usage();
    }
  }

  if(optind == argc){
    usage();
  }
  p = atoi(argv[optind]);
  if(p <= 0 || p > 65535){
    usage();
  }

  n = socket(AF_NETLINK, SOCK_RAW, NETLINK_INET_DIAG);
  if(n == -1){
    fprintf(stderr, "Can't Open NetLink!!\n");
    return(1);
  }
  if(s == 0){
    s = 0xFFF;
  }
  c = scount(n, p, s);
  if(c != -1){
    if (v) {
      showcount(s);
    } else {
      printf("%d\n", c);
    }
  }else{
    return(1);
  }
  return(0);
}

