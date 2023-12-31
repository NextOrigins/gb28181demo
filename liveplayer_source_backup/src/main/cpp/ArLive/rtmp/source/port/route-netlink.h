// https://www.linuxjournal.com/article/8498
// http://manpages.ubuntu.com/manpages/xenial/man7/rtnetlink.7.html

#include <sys/types.h>
#include <sys/socket.h>
#include <linux/rtnetlink.h>
#include <net/if.h> //for IF_NAMESIZ, route_info   
#include <stdlib.h>
#include <string.h>

static socket_t netlink_route()
{
	socket_t s;
	struct sockaddr_nl addr;

	s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);

	memset(&addr, 0, sizeof(addr));
	addr.nl_family = AF_NETLINK;
	if (0 != bind(s, (struct sockaddr*)&addr, sizeof(addr)))
		return socket_invalid;

	return s;
}

static int netlink_send(socket_t s, const void* data, size_t bytes)
{
	struct iovec vec;
	struct msghdr msg;
	struct sockaddr_nl addr;

	vec.iov_base = (void*)data;
	vec.iov_len = bytes;

	memset(&addr, 0, sizeof(addr));
	addr.nl_family = AF_NETLINK;

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = &addr;
	msg.msg_namelen = sizeof(addr);
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;

	return sendmsg(s, &msg, 0);
}

//static int netlink_send2(socket_t s, const struct nlmsghdr* hdr, const void* data, size_t bytes)
//{
//	struct iovec vec[2];
//	struct msghdr msg;
//	struct sockaddr_nl addr;
//
//	vec[0].iov_base = hdr;
//	vec[0].iov_len = sizeof(*hdr);
//	vec[1].iov_base = data;
//	vec[1].iov_len = bytes;
//
//	memset(&addr, 0, sizeof(addr));
//	addr.nl_family = AF_NETLINK;
//
//	memset(&msg, 0, sizeof(msg));
//	msg.msg_name = &addr;
//	msg.msg_namelen = sizeof(addr);
//	msg.msg_iov = vec;
//	msg.msg_iovlen = 2;
//
//	return sendmsg(s, &msg, 0);
//}

static int netlink_recv(socket_t s, void* data, size_t bytes)
{
	int r;
	struct iovec vec;
	struct msghdr msg;
	struct sockaddr_nl addr;

	vec.iov_base = data;
	vec.iov_len = bytes;

	memset(&addr, 0, sizeof(addr));
	addr.nl_family = AF_NETLINK;

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = &addr;
	msg.msg_namelen = sizeof(addr);
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;

	r = recvmsg(s, &msg, 0);
	if (msg.msg_flags & MSG_TRUNC)
		return -1;
	return r;
}

static int netlink_getgateway(socket_t s, const struct sockaddr* addr)
{
	size_t bytes;
	uint8_t* data;
	ssize_t attrlen;
	struct rtmsg* rt;
	struct rtattr* attr;
	struct nlmsghdr* hdr;
	ssize_t len;
	int ifidx = -1;

	assert(AF_INET == addr->sa_family || AF_INET6 == addr->sa_family);
	bytes = NLMSG_SPACE(sizeof(*rt) + RTA_SPACE(AF_INET == addr->sa_family ? 4 : 16));
	data = (uint8_t*)calloc(1, bytes + 2048);
	if (!data) goto out;

	hdr = (struct nlmsghdr*)data;
	hdr->nlmsg_len = NLMSG_LENGTH(sizeof(*rt) + RTA_SPACE(AF_INET == addr->sa_family ? 4 : 16));
	hdr->nlmsg_flags = NLM_F_REQUEST;// | NLM_F_DUMP;
	hdr->nlmsg_type = RTM_GETROUTE;

	rt = (struct rtmsg*)NLMSG_DATA(hdr);
	rt->rtm_family = addr->sa_family;
	rt->rtm_table = RT_TABLE_MAIN;
	rt->rtm_dst_len = 8 * (AF_INET == addr->sa_family ? 4 : 16);

	attr = (struct rtattr*)(rt + 1);
	attr->rta_type = RTA_DST;
	if (AF_INET == addr->sa_family)
	{
		attr->rta_len = RTA_LENGTH(4);
		memcpy(RTA_DATA(attr), &((struct sockaddr_in*)addr)->sin_addr.s_addr, 4);
	}
	else
	{
		attr->rta_len = RTA_LENGTH(16);
		memcpy(RTA_DATA(attr), &((struct sockaddr_in6*)addr)->sin6_addr.s6_addr, 16);
	}

	if (netlink_send(s, data, bytes) < 0)
		goto out;

	hdr = (struct nlmsghdr*)(data + bytes);
	len = netlink_recv(s, hdr, 2048);
	if (len < 0)
		goto out;

	for (; NLMSG_OK(hdr, len); hdr = NLMSG_NEXT(hdr, len))
	{
		/* Find the message(s) concerting the main routing table, each message
		 * corresponds to a single routing table entry.
		 */
		rt = (struct rtmsg *)NLMSG_DATA(hdr);
		if (rt->rtm_table != RT_TABLE_MAIN)
			continue;

		/* Parse all the attributes for a single routing table entry. */
		attrlen = RTM_PAYLOAD(hdr);
		for (attr = RTM_RTA(rt); RTA_OK(attr, attrlen); attr = RTA_NEXT(attr, attrlen))
		{
			if (attr->rta_type != RTA_OIF)
				continue;

			/* The output interface index. */
			ifidx = *(int*)RTA_DATA(attr);
			goto out;
		}
	}

out:
	if (data)
		free(data);
	return ifidx;
}

static int netlink_getaddr(socket_t s, int ifidx, int family, struct sockaddr_storage *addr)
{
	static const size_t addr_buffer_size = 1024;
	void *req = NULL;
	void *resp = NULL;
	struct nlmsghdr *hdr;
	struct ifaddrmsg *ifa;
	ssize_t len;
	size_t reqlen;
	int ret = -1;

	/* Allocate space for the request. */
	reqlen = NLMSG_SPACE(sizeof(*ifa));
	if ((req = calloc(1, reqlen)) == NULL)
		goto out; /* ENOBUFS */

	/* Build the RTM_GETADDR request.
	*
	* Querying specific indexes is broken, so do this the hard way.
	*/
	hdr = (struct nlmsghdr *)req;
	hdr->nlmsg_len = NLMSG_LENGTH(sizeof(*ifa));
	hdr->nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
	hdr->nlmsg_type = RTM_GETADDR;
	ifa = (struct ifaddrmsg *)NLMSG_DATA(hdr);
	ifa->ifa_family = family;
	ifa->ifa_index = ifidx;

	/* Issue the query. */
	if (netlink_send(s, req, reqlen) < 0)
		goto out;

	/* Allocate space for the response. */
	if ((resp = calloc(1, addr_buffer_size)) == NULL)
		goto out; /* ENOBUFS */

	/* Read the response(s).
	*
	* WARNING: All the packets generated by the request must be consumed (as in,
	* consume responses till NLMSG_DONE/NLMSG_ERROR is encountered).
	*/
	do {
		len = netlink_recv(s, resp, addr_buffer_size);
		if (len < 0)
			goto out;

		/* Parse the response payload into netlink messages. */
		for (hdr = (struct nlmsghdr *)resp; NLMSG_OK(hdr, len); hdr = NLMSG_NEXT(hdr, len)) {
			struct rtattr *attr;
			ssize_t attrlen;

			if (hdr->nlmsg_type == NLMSG_DONE) {
				goto out;
			}
			if (hdr->nlmsg_type == NLMSG_ERROR) {
				/* Even if we may have found the answer, something went wrong in the
				 * netlink layer, so return an error.
				 */
				ret = -1;
				goto out;
			}

			/* If we found the correct answer, skip parsing the attributes. */
			if (ret != -1)
				continue;

			/* Filter the interface based on family, index, and if it scoped
			 * correctly.
			 */
			ifa = (struct ifaddrmsg *)NLMSG_DATA(hdr);
			if (ifa->ifa_family != family || (int)ifa->ifa_index != ifidx)
				continue;
			if (ifa->ifa_scope != RT_SCOPE_UNIVERSE)
				continue;

			/* Parse all the attributes for a single interface entry. */
			attr = IFA_RTA(ifa);
			attrlen = RTM_PAYLOAD(hdr);
			for (; RTA_OK(attr, attrlen); attr = RTA_NEXT(attr, attrlen)) {
				if (attr->rta_type == IFA_ADDRESS) {
					memset(addr, 0, sizeof(*addr));
					if (family == AF_INET)
						memcpy(&((struct sockaddr_in*)addr)->sin_addr, RTA_DATA(attr), sizeof(struct in_addr));
					else
						memcpy(&((struct sockaddr_in6*)addr)->sin6_addr, RTA_DATA(attr), sizeof(struct in6_addr));
					addr->ss_family = family;
					ret = 0;
					break;
				}
			}
		}
	} while (1);

out:
	if (req)
		free(req);
	if (resp)
		free(resp);
	return ret;
}

static int router_gateway(const struct sockaddr* dst, struct sockaddr_storage* gateway)
{
	int ifidx;
	socket_t s;
	
	s = netlink_route();
	if (socket_invalid == s) return -1;

	ifidx = netlink_getgateway(s, dst);
	if (ifidx >= 0)
	{
		ifidx = netlink_getaddr(s, ifidx, dst->sa_family, gateway);
	}

	socket_close(s);
	return ifidx;
}
