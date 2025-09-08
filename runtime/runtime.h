#pragma once

// 统一包含所有runtime模块
#include "core/context.h"
#include "core/rule.h"
#include "core/engine.h"
#include "condition/condition_evaluator.h"
#include "condition/operators.h"
#include "expression/expression.h"
#include "priority/priority_manager.h"
#include "behavior_tree/behavior_tree.h"

// 全局函数声明
void UpdateTask();
void InitTask();
void ExecuteTask();