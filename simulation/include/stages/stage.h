#ifndef _STAGES_STAGE_H
#define _STAGES_STAGE_H

enum StageType {
	STAGE_TYPE_FLAG = 0xF000,
	STAGE_TYPE_PLAYER = 0x0000,
	STAGE_TYPE_OPPONENT = 0x1000,
	STAGE_TYPE_GAME_FLOW = 0x2000 // game flow including randoms
};

enum Stage {
	STAGE_UNKNOWN = 0,

	// player turns
	STAGE_PLAYER_CHOOSE_BOARD_MOVE = 0x0001,

	// opponent turns
	STAGE_OPPONENT_CHOOSE_BOARD_MOVE = 0x1001,

	// game flow
	STAGE_PLAYER_TURN_START = 0x2001,
	STAGE_PLAYER_TURN_END,
	STAGE_OPPONENT_TURN_START,
	STAGE_OPPONENT_TURN_END,
	STAGE_WIN,
	STAGE_LOSS,
};
#endif
