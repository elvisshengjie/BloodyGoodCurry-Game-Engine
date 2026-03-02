/*********************************************************************************************
 \file      DecisionNode.cpp
 \par       SofaSpuds
 \author   jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%

 \brief     Implementation of the DecisionNode class, which represents a single node in a
            decision tree. Each node evaluates a condition, executes an action, or branches
            to its true or false child node based on the condition result.

 \details
            The DecisionNode class enables the construction of dynamic decision trees for
            AI or gameplay logic. Each node can define:
            - A conditional function that determines the evaluation path.
            - Two child nodes for branching logic.
            - An action function executed at leaf nodes or when no branch is available.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#include "DecisionNode.h"
#include "Common/CRTDebug.h"   // <- bring in DBG_NEW

#ifdef _DEBUG
#define new DBG_NEW       // <- redefine new AFTER all includes
#endif

namespace Framework
{
     /*********************************************************************************************
     \brief
        Constructs a DecisionNode with a condition, true/false branches, and an optional action.

     \param condition
        Function that takes a float and returns a boolean to guide branching.
     \param trueNode
        Unique pointer to the node evaluated if the condition is true.
     \param falseNode
        Unique pointer to the node evaluated if the condition is false.
     \param leafAction
        Optional function executed if the node has no branches.
    *********************************************************************************************/
    DecisionNode::DecisionNode
    (
        Condition condition,
        std::unique_ptr<DecisionNode> trueNode,
        std::unique_ptr<DecisionNode> falseNode,
        Action leafAction
    )
        : mainqns(std::move(condition)),
        ifTrue(std::move(trueNode)),
        ifFalse(std::move(falseNode)),
        action(std::move(leafAction))
    {
    }

    /*********************************************************************************************
     \brief
        Evaluates the node and determines which branch or action to execute.

     \param dt
        Floating-point input parameter, often used as delta time or context value.

     \details
        - If a condition exists, it is evaluated to decide between true or false branches.
        - If no branches exist, the node�s action is executed instead.
        - If no condition is defined, the node directly performs its action.
    *********************************************************************************************/
    void DecisionNode::evaluate(Framework::BehaviorContext& ctx)
    {
        if (mainqns) {
            if (mainqns(ctx)) {
                if (ifTrue) ifTrue->evaluate(ctx);
                else if (action) action(ctx);
            }
            else {
                if (ifFalse) ifFalse->evaluate(ctx);
                else if (action) action(ctx);
            }
        }
        else if (action) {
            action(ctx);
        }
    }
}
