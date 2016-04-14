/*
 *    Copyright 2016 Nest Labs Inc. All Rights Reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <stdlib.h>

#include <cli/cli_ip.hpp>
#include <common/code_utils.hpp>

namespace Thread {
namespace Cli {

static const char kName[] = "ip";

Ip::Ip(Server &server):
    Command(server)
{
}

const char *Ip::GetName()
{
    return kName;
}

int Ip::PrintUsage(char *buf, uint16_t buf_length)
{
    char *cur = buf;
    char *end = cur + buf_length;

    snprintf(cur, end - cur,
             "usage: ip\r\n"
             "  addr add <addr> dev <dev>\r\n"
             "  addr del <addr> dev <dev>\r\n");
    cur += strlen(cur);

    return cur - buf;
}

ThreadError Ip::AddAddress(int argc, char *argv[])
{
    ThreadError error = kThreadError_Error;
    int argcur = 0;

    SuccessOrExit(error = m_address.address.FromString(argv[argcur]));
    VerifyOrExit(++argcur < argc, error = kThreadError_Parse);

    VerifyOrExit(strcmp(argv[argcur], "dev") == 0, error = kThreadError_Parse);
    VerifyOrExit(++argcur < argc, error = kThreadError_Parse);

    for (Netif *netif = Netif::GetNetifList(); netif; netif = netif->GetNext())
    {
        if (strcmp(netif->GetName(), argv[argcur]) == 0)
        {
            m_address.prefix_length = 64;
            m_address.preferred_lifetime = 0xffffffff;
            m_address.valid_lifetime = 0xffffffff;
            netif->AddUnicastAddress(m_address);
            ExitNow(error = kThreadError_None);
        }
    }

exit:
    return error;
}

ThreadError Ip::DeleteAddress(int argc, char *argv[])
{
    ThreadError error;
    int argcur = 0;

    Ip6Address address;

    SuccessOrExit(error = address.FromString(argv[argcur]));
    VerifyOrExit(++argcur < argc, error = kThreadError_Parse);
    VerifyOrExit(address == m_address.address, error = kThreadError_Error);

    VerifyOrExit(strcmp(argv[argcur], "dev") == 0, error = kThreadError_Parse);
    VerifyOrExit(++argcur < argc, error = kThreadError_Parse);

    for (Netif *netif = Netif::GetNetifList(); netif; netif = netif->GetNext())
    {
        if (strcmp(netif->GetName(), argv[argcur]) == 0)
        {
            SuccessOrExit(error = netif->RemoveUnicastAddress(m_address));
            ExitNow(error = kThreadError_None);
        }
    }

exit:
    return error;
}

void Ip::Run(int argc, char *argv[], Server &server)
{
    ThreadError error = kThreadError_Error;
    char buf[512];
    char *cur = buf;
    char *end = cur + sizeof(buf);

    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-h") == 0)
        {
            ExitNow(error = kThreadError_InvalidArgs);
        }
        else if (strcmp(argv[i], "addr") == 0)
        {
            VerifyOrExit(++i < argc, error = kThreadError_InvalidArgs);

            if (strcmp(argv[i], "add") == 0)
            {
                i++;
                ExitNow(error = AddAddress(argc - i, &argv[i]));
            }
            else if (strcmp(argv[i], "del") == 0)
            {
                i++;
                ExitNow(error = DeleteAddress(argc - i, &argv[i]));
            }
        }
    }

exit:

    if (error != kThreadError_None)
    {
        cur += PrintUsage(cur, end - cur);
    }

    snprintf(cur, end - cur, "Done\r\n");
    cur += strlen(cur);
    server.Output(buf, cur - buf);
}

}  // namespace Cli
}  // namespace Thread
