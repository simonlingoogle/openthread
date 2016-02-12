/*
 *
 *    Copyright (c) 2015 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    This document is the property of Nest. It is considered
 *    confidential and proprietary information.
 *
 *    This document may not be reproduced or transmitted in any form,
 *    in whole or in part, without the express written permission of
 *    Nest.
 *
 *    @author  Jonathan Hui <jonhui@nestlabs.com>
 *
 */

#ifndef CLI_CLI_IFCONFIG_H_
#define CLI_CLI_IFCONFIG_H_

#include <cli/cli_command.h>

namespace Thread {

class CliIfconfig: public CliCommand {
 public:
  explicit CliIfconfig(CliServer *server);
  const char *GetName() final;
  void Run(int argc, char *argv[], CliServer *server) final;
};

}  // namespace Thread

#endif  // CLI_CLI_IFCONFIG_H_
