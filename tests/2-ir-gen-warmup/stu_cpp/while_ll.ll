;ModuleID = 'assign.c'
source_filename = "assign.c"
; 注释: target的开始
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"
; 注释: target的结束

; 注释: 全局main函数的定义
; Function Attrs: noinline nounwind optnone uwtable

define i32 @main() {
label_entry:
  %op0 = alloca i32
  store i32 0, i32* %op0
  %op1 = alloca i32
  store i32 10, i32* %op1
  %op2 = alloca i32
  store i32 0, i32* %op2
  %op3 = load i32, i32* %op1
  %op4 = load i32, i32* %op2
  br label %label_loop
label_loop:                                                ; preds = %label_entry, %label_tureBB
  %op5 = load i32, i32* %op2
  %op6 = icmp slt i32 %op5, 10
  br i1 %op6, label %label_tureBB, label %label_falseBB
label_tureBB:                                                ; preds = %label_loop
  %op7 = load i32, i32* %op2
  %op8 = add i32 %op7, 1
  store i32 %op8, i32* %op2
  %op9 = load i32, i32* %op1
  %op10 = load i32, i32* %op2
  %op11 = add i32 %op10, %op9
  store i32 %op11, i32* %op1
  br label %label_loop
label_falseBB:                                                ; preds = %label_loop
  %op12 = load i32, i32* %op1
  store i32 %op12, i32* %op0
  br label %label13
label13:                                                ; preds = %label_falseBB
  %op14 = load i32, i32* %op0
  ret i32 %op14
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.1 "}