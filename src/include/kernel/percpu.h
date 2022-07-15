#ifndef __PERCPU_H__
#define __PERCPU_H__

#include <arch/amd64/cpu.h>

extern uint8_t _percpu_start, _percpu_end;

#pragma GCC push_options
#pragma GCC optimize ("O0")

#define __percpu_size           ((uint64_t)((uint64_t)&_percpu_end - (uint64_t)&_percpu_start))
#define __this_cpu_ptr(var)     ((typeof(var) *)(((uint8_t *)(&(var))) + get_msr(GS_BASE)))
#define __any_cpu_ptr(var, cpu) ((typeof(var) *)(((uint8_t *)(&(var))) + cpu * __percpu_size))

#define get_thiscpu_var(var) *(__this_cpu_ptr(var))
#define get_thiscpu_ptr(var)  (__this_cpu_ptr(var))
#define get_thiscpu_id()      (get_msr(GS_BASE) / __percpu_size)

#define put_thiscpu_var(var) ((void)(var))
#define put_thiscpu_ptr(var) ((void)(var))

#define get_percpu_var(var, cpu) *(__any_cpu_ptr(var, cpu))
#define get_percpu_ptr(var, cpu)  (__any_cpu_ptr(var, cpu))

#define put_percpu_var(var, cpu) ((void)(&(var)))
#define put_percpu_ptr(var, cpu) ((void)(var))

#define percpu_init(cpu)         (set_msr(GS_BASE, (cpu) * __percpu_size))

#pragma GCC pop_options

#endif /* __PERCPU_H__ */
