#pragma once

// 统一包含所有行为树模块
#include "bt_node.h"
#include "bt_parser.h"
#include "bt_executor.h"
#include "bt_manager.h"

// 便捷的类型别名
using BTNodePtr = shared_ptr<BTNode>;
using BTActionPtr = shared_ptr<BTAction>;
using BTConditionPtr = shared_ptr<BTCondition>;
using BTCompositePtr = shared_ptr<BTComposite>;
using BTDecoratorPtr = shared_ptr<BTDecorator>;
using BTExecutorPtr = shared_ptr<BTExecutor>;
using BTManagerPtr = shared_ptr<BTManager>;
