#pragma once

#include "Cards/id-map.h"

namespace FlowControl
{
	namespace Manipulators
	{
		namespace Helpers
		{
			class TransformHelper
			{
			public:
				TransformHelper(state::State & state, FlowControl::FlowContext & flow_context, state::CardRef card_ref, state::Cards::Card & card) :
					state_(state), flow_context_(flow_context), card_ref_(card_ref), card_(card)
				{
				}

				void Transform(Cards::CardId id);
				void BecomeCopyOf(state::Cards::CardData const& new_data);

			private:
				state::State & state_;
				FlowControl::FlowContext & flow_context_;
				state::CardRef card_ref_;
				state::Cards::Card & card_;
			};
		}
	}
}