// kernel Module for LRK_Proxy (Light RTP Kernel Proxy)
// Date: 03-01-2021
// By: Mojtaba Esfandiari , Maryam Baghdadi
// Company Name: Nasim Telecom
#include <linux/version.h>
#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/string.h>
#include <linux/hashtable.h>
#include <linux/kernel.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/socket.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/spinlock.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/time.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/stat.h>

//Default port range which dedicated by user programm and take as input arguments by kernel module
static long int min_port = 20000;
static long int max_port = 40000;

module_param(min_port, long, S_IRUSR);
MODULE_PARM_DESC(min_port, "min port range value");
module_param(max_port, long, S_IRUSR);
MODULE_PARM_DESC(max_port, "max port range value");


#define NETLINK_USER 31
struct sock *nl_sk = NULL;

// declare hooking structures for prerouting and postrouting
static struct nf_hook_ops netfilter_ops_post;
static struct nf_hook_ops netfilter_ops_pre;


DEFINE_HASHTABLE(netlink_message_table, 10);

// structure for Insert Msg content in hash table
struct netlink_message {

  /* For hash table */
  struct hlist_node hash_node;
  /* Keys */
  char session;
  __be32 src_ipv4;
  __be32 dst_ipv4;
  __be32 snat_ipv4;
  __be32 dnat_ipv4;
  __be16 src_port;
  __be16 dst_port;
  __be16 snat_port;
  __be16 dnat_port;
  char* timeout;
  char* callid;

};


void *malloc(size_t s) {
  	void *ptr = kmalloc(s, GFP_KERNEL);
	//printk(KERN_DEBUG "[CODA] Allocate memory %p\n", ptr);
  	return ptr;
}

void free(void *ptr) {
	//printk(KERN_DEBUG "[CODA] Deallocate memory %p\n", ptr);
	kfree(ptr);
}


/* update the checksums for udp and ip*/
void update_udp_ip_checksum(struct sk_buff *skb, struct udphdr *udph,
                            struct iphdr *iph)
{

    int len;
    if (!skb || !iph || !udph) return ;
    len = skb->len;

    if (unlikely(skb_linearize(skb) != 0)){
        pr_info("+++++++++++++++++++++++no linearize+++++++++++++++++++++++ \n");
    }

    /*update ip checksum*/
    iph->check = 0;
    iph->check = ip_fast_csum((unsigned char *)iph, iph->ihl);
    /*update udp checksum */
    udph->check = 0;
    udph->check = udp_v4_check(
            len - 4*iph->ihl,
            iph->saddr, iph->daddr,
            csum_partial((char *)udph, len-4*iph->ihl,
                         0));
    return;

}


/*helper routines for IP address conversion*/
unsigned long ip_asc_to_int(char *strip)
{
	unsigned long ip;
        unsigned int a[4];

        sscanf(strip, "%u.%u.%u.%u", &a[0], &a[1], &a[2], &a[3]);
        ip = (a[0] << 24)+(a[1] << 16)+(a[2] << 8)+a[3] ;
	return ip;
}
/*PRE ROUTING Hook: In this we do DNAT
For packets coming from WAN, destination IP and port are changed to lan ip and port from NAT Table entries */
unsigned int main_hook_pre(void *hooknum, struct sk_buff *skb, const struct nf_hook_state *hstate) {

	struct iphdr *iph;
    	struct udphdr *udph;

	 struct netlink_message *netlink_message;
    	if (!skb) return NF_ACCEPT;
    	iph = ip_hdr(skb);

    	if (!iph) return NF_ACCEPT;

    	if (iph->protocol == IPPROTO_UDP)
	{
        	udph = (struct udphdr *) ((char *) iph + iph->ihl * 4);
        	if (!udph) return NF_ACCEPT;

		// lookup in hash table according to destination port as key value
    		hash_for_each_possible(netlink_message_table,netlink_message , hash_node, udph->dest) {

			// modify destination IP address before forwarding
			iph->daddr=netlink_message->src_ipv4;

			update_udp_ip_checksum(skb, udph, iph);

    		}

	}
    	return NF_ACCEPT;

}
/*POST ROUTING hook: We do SNAT here.
Packets from LAN - source IP and port are translated to public IP and sent out*/
unsigned int main_hook_post(void *hooknum, struct sk_buff *skb, const struct nf_hook_state *hstate) {

	struct iphdr *iph;
    	struct udphdr *udph;
	struct netlink_message *netlink_message;

	if (!skb) return NF_ACCEPT;

    	iph = ip_hdr(skb);

    	if (!iph) return NF_ACCEPT;

    	if (iph->protocol == IPPROTO_UDP) {

        	udph = (struct udphdr *) ((char *) iph + iph->ihl * 4);
        	if (!udph) return NF_ACCEPT;

		// lookup in hash table according to destination port as key value
    		hash_for_each_possible(netlink_message_table,netlink_message , hash_node, udph->dest) {

			// modify source IP address, destination and source  port Numbers before packet forwarding

			// Note: We can't modify source IP address in pre routing function because forwarding
			// doesn't happen if snat IP address is locat system IP address

			iph->saddr=netlink_message->snat_ipv4;
			udph->dest = netlink_message->src_port;
			udph->source = netlink_message->snat_port;

			update_udp_ip_checksum(skb, udph, iph);
		}
         }

    return NF_ACCEPT;
}


// Procedure of Receiving Msg from User Space and Insert to hash_table
static void lrkp_nl_recv_msg(struct sk_buff *skb)
{
	struct nlmsghdr *nlh;
	int msg_size;

	char* session=NULL;
  	__be32 src_ipv4;
  	__be32 dst_ipv4;
  	__be32 snat_ipv4;
  	__be32 dnat_ipv4;
  	__be16 src_port;
  	__be16 dst_port;
  	__be16 snat_port;
  	__be16 dnat_port;
  	char* timeout=NULL;
  	char* callid=NULL;

  	int cnt=0;
  	struct netlink_message *netlink_message_s1;
  	struct netlink_message *netlink_message_s2;
 	__be16 key_s1;
  	__be16 key_s2;
  	char* const delim = " ";
  	char* token;
	char* cur;
	int val;

	//RTCP parameters declaration
	struct netlink_message *netlink_message_rtcp1;
  	struct netlink_message *netlink_message_rtcp2;
  	struct netlink_message *netlink_message_for_free;
 	__be16 key_rtcp1;
  	__be16 key_rtcp2;
   	__be16 rtcp_src_port;
  	__be16 rtcp_dst_port;
  	__be16 rtcp_snat_port;
  	__be16 rtcp_dnat_port;


	printk(KERN_INFO "Entering: %s\n", __FUNCTION__);


	nlh=(struct nlmsghdr*)skb->data;
	cur = (char*)nlmsg_data(nlh);//strtok((char*)nlmsg_data(nlh));
	//printk(KERN_INFO "Netlink successfull received msg payload:%s\n",(char*)nlmsg_data(nlh));
	//printk(KERN_INFO "Netlink received msg successfully\n");

	// get message and extract data
	msg_size=strlen(cur);
	if (msg_size>0) {

		while((token= strsep(&cur,delim)))
		{
			if(cnt==1)
				session = token;
			if(cnt==2)
				src_ipv4=ip_asc_to_int(token);
			if(cnt==3)
				dst_ipv4 = ip_asc_to_int(token);
			if(cnt==4)
				snat_ipv4 = ip_asc_to_int(token);
			if(cnt==5)
				dnat_ipv4 = ip_asc_to_int(token);
			if(cnt==6)
			{
				kstrtoint(token, 10, &val);
				src_port = htons(val);

				rtcp_src_port = htons(val+1);
			}
			if(cnt==7)
			{
				kstrtoint(token, 10, &val);
				dst_port = htons(val);
				key_s1 = htons(val);

				rtcp_dst_port = htons(val+1);
				key_rtcp1 = htons(val+1);
			}
			if(cnt==8)
			{
				kstrtoint(token, 10, &val);
				snat_port = htons(val);
				key_s2 = htons(val);

				rtcp_snat_port = htons(val+1);
				key_rtcp2 = htons(val+1);
			}
			if(cnt==9)
			{
				kstrtoint(token, 10, &val);
				dnat_port = htons(val);

				rtcp_dnat_port = htons(val+1);
			}
			if(cnt==10)
				timeout = token;
			if(cnt==11)
				callid = token;

			cnt++;
 		}

		// delete allocated memory from hash table with key destination port
    		hash_for_each_possible(netlink_message_table,netlink_message_s1 , hash_node, key_s1) {
			hash_del_rcu(&netlink_message_s1->hash_node);
			free(netlink_message_s1);
		}

		// delete allocated memory from hash table with key snat port
    		hash_for_each_possible(netlink_message_table,netlink_message_s2 , hash_node, key_s2) {
			hash_del_rcu(&netlink_message_s2->hash_node);
			free(netlink_message_s2);
		}


		netlink_message_s1 = (struct netlink_message*) kmalloc(sizeof(struct netlink_message)*150, GFP_KERNEL);
    		if (netlink_message_s1 == NULL) {
      			printk(KERN_ALERT "[CODA] Cannot alloc netlink_message_s1");
      			return NF_ACCEPT;
    		}

    		netlink_message_s2 = (struct netlink_message*) kmalloc(sizeof(struct netlink_message)*150, GFP_KERNEL);
		if (netlink_message_s2 == NULL) {
      			printk(KERN_ALERT "[CODA] Cannot alloc netlink_message_s2");
      			return NF_ACCEPT;
    		}


  		// define hash & Insert in sender side
		netlink_message_s1->src_ipv4 = htonl(src_ipv4);
    		netlink_message_s1->dst_ipv4 = htonl(dst_ipv4);
    		netlink_message_s1->snat_ipv4 = htonl(snat_ipv4);
    		netlink_message_s1->dnat_ipv4 = htonl(dnat_ipv4);
    		netlink_message_s1->src_port = src_port;
    		netlink_message_s1->dst_port = dst_port;
    		netlink_message_s1->snat_port = snat_port;
    		netlink_message_s1->dnat_port = dnat_port;
    		netlink_message_s1->timeout = timeout;
    		netlink_message_s1->callid = callid;
    		netlink_message_s1->session = session[0];
   		// Insert To hash_table with key src_port
		hash_add_rcu(netlink_message_table, &netlink_message_s1->hash_node, key_s1);

  		// define hash & Insert in receiver side
		netlink_message_s2->src_ipv4 = htonl(dnat_ipv4);
    		netlink_message_s2->dst_ipv4 = htonl(snat_ipv4);
    		netlink_message_s2->snat_ipv4 = htonl(dst_ipv4);
    		netlink_message_s2->dnat_ipv4 = htonl(src_ipv4);
    		netlink_message_s2->src_port = dnat_port;
    		netlink_message_s2->dst_port = snat_port;
    		netlink_message_s2->snat_port = dst_port;
    		netlink_message_s2->dnat_port = src_port;
    		netlink_message_s2->timeout = timeout;
    		netlink_message_s2->callid = callid;
    		netlink_message_s2->session = session[0];

		// Insert To hash_table with key src_port
		hash_add_rcu(netlink_message_table, &netlink_message_s2->hash_node, key_s2);


		// RTCP procedure for allocation & insert in hash table

		// delete allocated memory from hash table for rtcp messages with key destination port
    		hash_for_each_possible(netlink_message_table,netlink_message_rtcp1 , hash_node, key_rtcp1) {
			hash_del_rcu(&netlink_message_rtcp1->hash_node);
			free(netlink_message_rtcp1);
		}

		// delete allocated memory from hash table for rtcp messages with key snat port
    		hash_for_each_possible(netlink_message_table,netlink_message_rtcp2 , hash_node, key_rtcp2) {
			hash_del_rcu(&netlink_message_rtcp2->hash_node);
			free(netlink_message_rtcp2);
		}


		netlink_message_rtcp1 = (struct netlink_message*) kmalloc(sizeof(struct netlink_message)*150, GFP_KERNEL);
    		if (netlink_message_rtcp1 == NULL) {
      			printk(KERN_ALERT "[CODA] Cannot alloc netlink_message_rtcp1");
      			return NF_ACCEPT;
    		}

    		netlink_message_rtcp2 = (struct netlink_message*) kmalloc(sizeof(struct netlink_message)*150, GFP_KERNEL);
		if (netlink_message_rtcp2 == NULL) {
      			printk(KERN_ALERT "[CODA] Cannot alloc netlink_message_rtcp2");
      			return NF_ACCEPT;
    		}


  		// define hash & Insert in sender side for RTCP
		netlink_message_rtcp1->src_ipv4 = htonl(src_ipv4);
    		netlink_message_rtcp1->dst_ipv4 = htonl(dst_ipv4);
    		netlink_message_rtcp1->snat_ipv4 = htonl(snat_ipv4);
    		netlink_message_rtcp1->dnat_ipv4 = htonl(dnat_ipv4);
    		netlink_message_rtcp1->src_port = rtcp_src_port;
    		netlink_message_rtcp1->dst_port = rtcp_dst_port;
    		netlink_message_rtcp1->snat_port = rtcp_snat_port;
    		netlink_message_rtcp1->dnat_port = rtcp_dnat_port;
    		netlink_message_rtcp1->timeout = timeout;
    		netlink_message_rtcp1->callid = callid;
    		netlink_message_rtcp1->session = session[0];
   		// Insert To hash_table with key src_port
		hash_add_rcu(netlink_message_table, &netlink_message_rtcp1->hash_node, key_rtcp1);

  		// define hash & Insert in receiver side for RTCP
		netlink_message_rtcp2->src_ipv4 = htonl(dnat_ipv4);
    		netlink_message_rtcp2->dst_ipv4 = htonl(snat_ipv4);
    		netlink_message_rtcp2->snat_ipv4 = htonl(dst_ipv4);
    		netlink_message_rtcp2->dnat_ipv4 = htonl(src_ipv4);
    		netlink_message_rtcp2->src_port = rtcp_dnat_port;
    		netlink_message_rtcp2->dst_port = rtcp_snat_port;
    		netlink_message_rtcp2->snat_port = rtcp_dst_port;
    		netlink_message_rtcp2->dnat_port = rtcp_src_port;
    		netlink_message_rtcp2->timeout = timeout;
    		netlink_message_rtcp2->callid = callid;
    		netlink_message_rtcp2->session = session[0];
   		// Insert To hash_table with key src_port
		hash_add_rcu(netlink_message_table, &netlink_message_rtcp2->hash_node, key_rtcp2);

	}

}

// init function of module
static int __init lrkp_init(void) {

	//This is for 3.6 kernels and above.

	//input parameters
	struct netlink_kernel_cfg cfg = {
    		.input = lrkp_nl_recv_msg,
	};

	printk(KERN_INFO "load LRKProxy Module--------------------------------\n");
	nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
	if(!nl_sk)
	{
    		printk(KERN_ALERT "Error creating socket.\n");
    		return -10;
	}
	// NIC hook Postrouting Procedure
	netfilter_ops_post.hook = main_hook_post;
	netfilter_ops_post.pf = PF_INET;
        netfilter_ops_post.hooknum = NF_INET_POST_ROUTING;
        netfilter_ops_post.priority = NF_IP_PRI_FIRST;
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,13,0)
		nf_register_net_hook(&init_net, &netfilter_ops_post);
	#else
		nf_register_hook(&netfilter_ops_post);
	#endif

	// NIC hook Prerouting Procedure
	netfilter_ops_pre.hook = main_hook_pre;
    	netfilter_ops_pre.pf = PF_INET;
    	netfilter_ops_pre.hooknum = NF_INET_PRE_ROUTING;
    	netfilter_ops_pre.priority = NF_IP_PRI_FIRST;
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,13,0)
    		nf_register_net_hook(&init_net, &netfilter_ops_pre);
	#else
		nf_register_hook(&netfilter_ops_pre);
	#endif
	return 0;
}
// exit function of module
static void __exit lrkp_exit(void)
{
	struct netlink_message *netlink_message; /* Cursor for looping */
	struct hlist_node *temp_node; /* Temporary storage */
	int bkt;

	//free hash_table
  	hash_for_each_safe(netlink_message_table, bkt, temp_node, netlink_message, hash_node) {
    		hash_del_rcu(&netlink_message->hash_node);
    		free(netlink_message);
    		netlink_message = NULL;
  	}
	netlink_kernel_release(nl_sk);

	// unregister NIC hook postrouting
	// unregister NIC hook prerouting

	#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,13,0)
		nf_unregister_net_hook(&init_net, &netfilter_ops_post);
		nf_unregister_net_hook(&init_net, &netfilter_ops_pre);
	#else
		nf_unregister_hook(&netfilter_ops_post);
		nf_unregister_hook(&netfilter_ops_pre);
	#endif
	printk(KERN_INFO "unload LRKProxy module-------------------------------\n");
}

module_init(lrkp_init);
module_exit(lrkp_exit);

MODULE_LICENSE("GPL");
