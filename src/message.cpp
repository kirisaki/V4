#include "v4/errors.hpp"
#include "v4/internal/task_backend.h"
#include "v4/internal/vm.h"
#include "v4/task.h"

/* ========================================================================= */
/* Message Passing API (Backend Wrapper)                                    */
/* ========================================================================= */
/**
 * This file provides the public message passing API.
 * All implementation details are delegated to the backend layer.
 */

extern "C" v4_err vm_task_send(Vm *vm, uint8_t target_task, uint8_t msg_type,
                               int32_t data)
{
  return v4_backend_task_send(vm, target_task, msg_type, data);
}

extern "C" int vm_task_receive(Vm *vm, uint8_t msg_type, int32_t *data, uint8_t *src_task)
{
  return v4_backend_task_receive(vm, msg_type, data, src_task);
}

extern "C" int vm_task_receive_blocking(Vm *vm, uint8_t msg_type, int32_t *data,
                                        uint8_t *src_task, uint32_t timeout_ms)
{
  return v4_backend_task_receive_blocking(vm, msg_type, data, src_task, timeout_ms);
}
