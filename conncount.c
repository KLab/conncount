#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<getopt.h>
#include<sys/socket.h>
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

void usage()
{
  printf("usage: conncount port\n");
  exit(0);
}

int scount(int n, int p)
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

  wbuf.nlh.nlmsg_len          = NLMSG_ALIGN(NLMSG_LENGTH(sizeof(wbuf.req)));
  wbuf.nlh.nlmsg_type         = TCPDIAG_GETSOCK;
  wbuf.nlh.nlmsg_flags        = NLM_F_REQUEST | NLM_F_DUMP;
  wbuf.nlh.nlmsg_seq          = seq++;
  wbuf.req.idiag_family       = AF_INET;
  wbuf.req.idiag_src_len      = 0;
  wbuf.req.idiag_dst_len      = 0;
  wbuf.req.idiag_ext          = 0; 
  wbuf.req.idiag_states       = TCPF_ESTABLISHED;
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

  struct option opt[]={
    {"help",    0, NULL, 'h'},
    {0, 0, 0, 0}
  };

  while((r=getopt_long(argc, argv, "h", opt, NULL)) != -1){
    switch(r){
      case 'h':
        usage();

      case '?':
        usage();
    }
  }

  if(argc != 2){
    usage();
  }
  p = atoi(argv[1]);
  if(p <= 0 || p > 65535){
    usage();
  }

  n = socket(AF_NETLINK, SOCK_RAW, NETLINK_INET_DIAG);
  if(n == -1){
    fprintf(stderr, "Can't Open NetLink!!\n");
    return(1);
  }
  c = scount(n, p);
  if(c != -1){
    printf("%d\n", c);
  }else{
    return(1);
  }
  return(0);
}

