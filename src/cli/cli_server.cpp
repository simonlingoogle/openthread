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

/**
 * @file
 *   This file implements adding a CLI command to the CLI server.
 */

#include <cli/cli_command.hpp>
#include <common/code_utils.hpp>

namespace Thread {
namespace Cli {

ThreadError Server::Add(Command &command)
{
    ThreadError error = kThreadError_None;
    Command *cur;

    for (cur = mCommands; cur; cur = cur->GetNext())
    {
        if (cur == &command)
        {
            ExitNow(error = kThreadError_Busy);
        }
    }

    if (mCommands == NULL)
    {
        mCommands = &command;
    }
    else
    {
        for (cur = mCommands; cur->GetNext(); cur = cur->GetNext()) {}

        cur->SetNext(command);
    }

exit:
    return error;
}

}  // namespace Cli
}  // namespace Thread
