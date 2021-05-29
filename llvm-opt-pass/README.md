## Minimal Pass to Change Direct Calls to function
```bash
diff target-program.ll target_changed.ll 
1c1,7
< ; ModuleID = 'target-program.cpp'
---
> running code analysis...
> found a direct call to 'foo()'!
> found a direct call to 'foo()'!
> applying code transformation...
> the tranformed program:
> -------------
> ; ModuleID = 'target-program.ll'
32c38
<   call void @_Z3foov()
---
>   call void @_Z3bari(i32 1)
46c52
<   call void @_Z3foov()
---
>   call void @_Z3bari(i32 2)

```
void foo() is being changed to void bar(int i) where i
is set to number of function replaced till now.(So total 2 replacement).