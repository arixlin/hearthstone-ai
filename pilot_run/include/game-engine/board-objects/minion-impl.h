#include "game-engine/board.h"
#include "game-engine/board-objects/minion-data.h"
#include "game-engine/board-objects/minion.h"

inline GameEngine::Minion::Minion(Minion && rhs)
	: minions(rhs.minions), minion(std::move(rhs.minion)),
	enchantments(*this),
	auras(*this)
{
	// If minion has enchantments, it cannot be moved
	// TODO: we can use a unique_ptr to wrap the enchantments class,
	//       so we can support move
	// Same for auras
	if (!rhs.enchantments.Empty()) throw std::runtime_error("You should not move minion with enchantments");
	if (!rhs.auras.Empty()) throw std::runtime_error("You should not move minion with auras");
}

inline GameEngine::Minion::Minion(Minions & minions, Minion const & rhs)
	: minions(minions), minion(rhs.minion), enchantments(*this), auras(*this)
{
	// If minion has enchantments/auras, then it cannot be cloned
	// Note: The only chance we need to copy minion is to copy root node board in MCTS
	// If root node board has enchantments/auras, then ask the caller to prepare the root node board again
	// TODO: how about faceless manipulator?
	if (!rhs.enchantments.Empty()) throw std::runtime_error("You should not copy minion with enchantments");
	if (!rhs.auras.Empty()) throw std::runtime_error("You should not move minion with auras");
}

inline GameEngine::Minion::Minion(Minions & minions, Minion && minion)
	: minions(minions), minion(std::move(minion.minion)),
	enchantments(*this),
	auras(*this)
{
	// If minion has enchantments, it cannot be moved
	// TODO: we can use a unique_ptr to wrap the enchantments class,
	//       so we can support move
	if (!minion.enchantments.Empty()) throw std::runtime_error("You should not move minion with enchantments");
	if (!minion.auras.Empty()) throw std::runtime_error("You should not move minion with auras");
}

inline GameEngine::Minion::Minion(Minions & minions, MinionData const & minion)
	: minions(minions), minion(minion), enchantments(*this), auras(*this)
{
}

inline GameEngine::Minion::Minion(Minions & minions, MinionData && minion)
	: minions(minions), minion(std::move(minion)), enchantments(*this), auras(*this)
{
}

inline void GameEngine::Minion::DestroyBoard()
{
	this->enchantments.DestroyBoard();
	this->auras.DestroyBoard();
}

inline GameEngine::Board & GameEngine::Minion::GetBoard() const
{
	return this->minions.GetBoard();
}

inline int GameEngine::Minion::GetHP() const
{
	return this->minion.stat.GetHP();
}

inline int GameEngine::Minion::GetMaxHP() const
{
	return this->minion.stat.GetMaxHP();
}

inline int GameEngine::Minion::GetAttack() const
{
	return this->minion.stat.GetAttack();
}

inline void GameEngine::Minion::TakeDamage(int damage, bool poisonous)
{
	if (this->minion.stat.IsShield()) {
		this->minion.stat.ClearShield();
	}
	else {
		this->minion.stat.SetHP(this->minion.stat.GetHP() - damage);

		if (poisonous) {
			this->GetMinions().MarkPendingRemoval(*this);
		}

		this->HookMinionCheckEnraged();
		this->GetBoard().hook_manager.HookAfterMinionDamaged(*this, damage);
	}
}

inline int GameEngine::Minion::GetForgetfulCount() const
{
	return this->minion.stat.GetForgetfulCount();
}

inline bool GameEngine::Minion::Attackable() const
{
	if (this->minion.summoned_this_turn && !this->minion.stat.IsCharge()) return false;

	if (this->minion.stat.IsFreezed()) return false;

	int max_attacked_times = 1;
	if (this->minion.stat.IsWindFury()) max_attacked_times = 2;

	if (this->minion.attacked_times >= max_attacked_times) return false;

	if (this->minion.stat.GetAttack() <= 0) return false;

	return true;
}

inline void GameEngine::Minion::AttackedOnce()
{
	this->minion.attacked_times++;
	if (this->minion.stat.IsStealth()) this->minion.stat.ClearStealth();
}

inline void GameEngine::Minion::SetFreezed()
{
	this->minion.stat.SetFreezed();
}

inline bool GameEngine::Minion::IsFreezeAttacker() const
{
	return this->minion.stat.IsFreezeAttacker();
}

inline bool GameEngine::Minion::IsFreezed() const
{
	return this->minion.stat.IsFreezed();
}

inline bool GameEngine::Minion::IsPoisonous() const
{
	return this->minion.stat.IsPoisonous();
}

inline void GameEngine::Minion::Heal(int amount)
{
	int origin_hp = this->minion.stat.GetHP();
	int max_hp = this->minion.stat.GetMaxHP();
	int new_hp = origin_hp + amount;
	if (new_hp > max_hp) new_hp = max_hp;
	this->minion.stat.SetHP(new_hp);

	// TODO: trigger
}

inline void GameEngine::Minion::AddAttack(int val)
{
	this->minion.stat.SetAttack(this->minion.stat.GetAttack() + val);
}

inline void GameEngine::Minion::IncreaseCurrentAndMaxHP(int val)
{
#ifdef DEBUG
	if (val < 0) throw std::runtime_error("should we trigger heal? enrage effect? damaged effect? use TakeDamage() for that.");
#endif
	this->minion.stat.SetMaxHP(this->minion.stat.GetMaxHP() + val);
	this->minion.stat.SetHP(this->minion.stat.GetHP() + val);

	// no need to check enrage, since we add the hp and max-hp by the same amount
}

inline void GameEngine::Minion::DecreaseMaxHP(int val)
{
	this->minion.stat.SetMaxHP(this->minion.stat.GetMaxHP() - val);
	this->minion.stat.SetHP(std::min(this->minion.stat.GetHP(), this->minion.stat.GetMaxHP()));

	this->HookMinionCheckEnraged(); // might become un-enraged if max-hp lowered to current-hp
}

inline void GameEngine::Minion::AddSpellDamage(int val)
{
	this->minion.stat.SetSpellDamage(this->minion.stat.GetSpellDamage() + val);
}

inline void GameEngine::Minion::AddOnDeathTrigger(OnDeathTrigger && func)
{
	this->minion.triggers_on_death.push_back(std::move(func));
}

inline std::list<GameEngine::Minion::OnDeathTrigger> GameEngine::Minion::GetAndClearOnDeathTriggers()
{
	std::list<GameEngine::Minion::OnDeathTrigger> ret;
	this->minion.triggers_on_death.swap(ret);
	return ret;
}

inline void GameEngine::Minion::SetMinionStatFlag(MinionStat::Flag flag)
{
	this->minion.stat.SetFlag(flag);
}

inline void GameEngine::Minion::RemoveMinionStatFlag(MinionStat::Flag flag)
{
	this->minion.stat.RemoveFlag(flag);
}

inline void GameEngine::Minion::ClearMinionStatFlag(MinionStat::Flag flag)
{
	this->minion.stat.ClearFlag(flag);
}

inline void GameEngine::Minion::Transform(MinionData const & minion_)
{
	this->minion = minion_;
}

inline void GameEngine::Minion::HookAfterMinionAdded(Minion & added_minion)
{
	this->auras.HookAfterMinionAdded(added_minion);
}

inline void GameEngine::Minion::HookMinionCheckEnraged()
{
	if (this->GetHP() < this->GetMaxHP()) {
		this->auras.HookAfterOwnerEnraged(); // enraged
	}
	else if (this->GetHP() == this->GetMaxHP()) {
		this->auras.HookAfterOwnerUnEnraged(); // un-enraged
	}
	else {
		throw std::runtime_error("hp should not be larger than max-hp");
	}
}

inline void GameEngine::Minion::HookAfterMinionDamaged(Minion & minion_, int damage)
{
	this->auras.HookAfterMinionDamaged(minion_, damage);
}

inline void GameEngine::Minion::HookBeforeMinionTransform(Minion & minion_, int new_card_id)
{
	this->auras.HookBeforeMinionTransform(minion_, new_card_id);
}

inline void GameEngine::Minion::HookAfterMinionTransformed(Minion & minion_)
{
	this->auras.HookAfterMinionTransformed(minion_);
}

inline void GameEngine::Minion::TurnStart(bool owner_turn)
{
	(void)owner_turn;
	this->minion.summoned_this_turn = false;
	this->minion.attacked_times = 0;

	this->auras.TurnStart(owner_turn);
}

inline void GameEngine::Minion::TurnEnd(bool owner_turn)
{
	if (owner_turn) {
		// check thaw
		// Note: if summon in this turn, and freeze it, then the minion will not be unfrozen
		if (this->minion.attacked_times == 0 && !this->minion.summoned_this_turn) {
			if (this->IsFreezed()) this->minion.stat.ClearFreezed();
		}
	}

	this->enchantments.TurnEnd();

	this->auras.TurnEnd(owner_turn);
}

inline GameEngine::Player & GameEngine::Minion::GetPlayer() const
{
	if (this->IsPlayerSide()) return this->GetBoard().player;
	else return this->GetBoard().opponent;
}

inline bool GameEngine::Minion::IsPlayerSide() const
{
	return &this->GetBoard().player.minions == &this->minions;
}

inline bool GameEngine::Minion::IsOpponentSide() const
{
	return &this->GetBoard().opponent.minions == &this->minions;
}