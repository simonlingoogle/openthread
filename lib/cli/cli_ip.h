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

#ifndef CLI_IP_H_
#define CLI_IP_H_

#include <cli/cli_command.h>
#include <net/netif.h>

namespace Thread {
namespace Cli {

class Ip: public Command
{
public:
    explicit Ip(Server &server);
    const char *GetName() final;
    void Run(int argc, char *argv[], Server &server) final;

private:
    int PrintUsage(char *buf, uint16_t buf_length);
    ThreadError AddAddress(int argc, char *argv[]);
    ThreadError DeleteAddress(int argc, char *argv[]);

    NetifUnicastAddress m_address;
};

}  // namespace Cli
}  // namespace Thread

#endif  // CLI_IP_H_
