#pragma once
#include "transfer/session.h"

namespace test
{

    void deliver(transfer::TransferSession& from, transfer::TransferSession& to, uint64_t now_ms);

}