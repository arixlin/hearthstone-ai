#pragma once

#include <assert.h>
#include "MCTS/Config.h"
#include "MCTS/selection/TreeNode.h"
#include "MCTS/selection/TraversedNodeInfo.h"

#include "MCTS/selection/ChildNodeMap-impl.h"

namespace mcts
{
	namespace selection
	{
		class Selection
		{
		public:
			Selection(std::mt19937 & rand) :
				path_(), random_(rand), policy_(), new_node_created_(false), pending_randoms_(false)
			{}

			Selection(Selection const&) = delete;
			Selection & operator=(Selection const&) = delete;

			void StartNewMainAction(TreeNode * root) {
				path_.clear();
				path_.emplace_back(root);
				new_node_created_ = false;
				pending_randoms_ = false;
			}

			// @return >= 0 for the chosen action
			int ChooseAction(
				engine::view::Board const& board,
				engine::ActionType action_type,
				engine::ActionChoices const& choices)
			{
				assert(!choices.Empty());

				if (action_type.IsChosenRandomly()) {
					assert(choices.CheckType<engine::ActionChoices::ChooseFromZeroToExclusiveMax>());
					pending_randoms_ = true;
					return random_.GetRandom(choices.Size());
				}

				assert(action_type.IsChosenManually());
				assert(!path_.empty());
				if (path_.back().HasMadeChoice()) {
					TreeNode* new_node = path_.back().ConstructNextNode(&new_node_created_);
					assert(new_node);
					path_.emplace_back(new_node);
				}

				assert(!path_.back().HasMadeChoice());
				TreeNode* current_node = path_.back().GetNode();

				if (pending_randoms_) {
					switch (action_type.GetType()) {
					case engine::ActionType::kChooseOne:
						// choose-one card might be triggered after random
						break;

					default:
						// Not allow random actions to be conducted first before any sub-action
						// That is, the player
						//    1. Choose the hand card
						//    2. Choose minion put location
						// Then, random gets started
						// Similar to other sub-actions
						assert(false);
						throw std::runtime_error("not allow random to be conducted before any sub-action.");
					}

					// Check if the board-view and sub-action-type are consistency
					// Currently, we have an assumption that
					//    * A random action can be swapped to the end of a sub-action sequence
					//      without any distinguishable difference
					// But, it may not be this case if...
					//    1. You have a 50% chance to choose a card
					//    2. Summon a random minion, then choose a card
					// In both cases, a random node should be added before the sub-action 'choose-a-card'
					// From a game-play view, that is to say
					//    The random outcome *influences* the player's decision
					// So, if this assertion failed
					//    It means a random node is necessary before this node
					assert(current_node->GetAddon().consistency_checker.SetAndCheck(board, action_type, choices));
				}
				else {
					assert(current_node->GetAddon().consistency_checker.SetAndCheck(board, action_type, choices));
				}

				int next_choice = current_node->Select(action_type, choices, policy_);
				assert(next_choice >= 0); // should report a valid action

				path_.back().MakeChoice(next_choice);

				return next_choice;
			}

			selection::TreeNode * FinishMainAction(engine::view::Board const& board, detail::BoardNodeMap * turn_node_map_, engine::Result result) {
				// mark the last action as a redirect node
				path_.back().ConstructRedirectNode();

				if (result != engine::kResultNotDetermined) {
					return nullptr;
				}

				bool new_node_created = false;
				selection::TreeNode * next_node = turn_node_map_->GetOrCreateNode(board, &new_node_created);
				assert(next_node);
				assert([&](selection::TreeNode* node) {
					if (!node->GetActionType().IsValid()) return true; // TODO: should not in this case?
					return node->GetActionType().GetType() == engine::ActionType::kMainAction;
				}(next_node));

				if (new_node_created) new_node_created_ = true;

				return next_node;
			}

			std::vector<TraversedNodeInfo> const& GetTraversedPath() const { return path_; }
			std::vector<TraversedNodeInfo> & GetMutableTraversedPath() { return path_; }

			bool HasNewNodeCreated() const { return new_node_created_; }

		private:
			std::vector<TraversedNodeInfo> path_;
			StaticConfigs::SelectionPhaseRandomActionPolicy random_;
			StaticConfigs::SelectionPhaseSelectActionPolicy policy_;
			bool new_node_created_;
			bool pending_randoms_;
		};
	}
}
