
#include "Globals.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "Sheep.h"
#include "../BlockID.h"
#include "../Entities/Player.h"
#include "../World.h"
#include "../EffectID.h"
#include "FastRandom.h"





cSheep::cSheep(int a_Color) :
	super("Sheep", mtSheep, "mob.sheep.say", "mob.sheep.say", 0.6, 1.3),
	m_IsSheared(false),
	m_WoolColor(a_Color),
	m_TimeToStopEating(-1)
{
	// Generate random wool color.
	if (m_WoolColor == -1)
	{
		m_WoolColor = GenerateNaturalRandomColor();
	}

	if ((m_WoolColor < 0) || (m_WoolColor > 15))
	{
		m_WoolColor = 0;
	}
}





void cSheep::GetDrops(cItems & a_Drops, cEntity * a_Killer)
{
	if (!m_IsSheared)
	{
		a_Drops.push_back(cItem(E_BLOCK_WOOL, 1, static_cast<short>(m_WoolColor)));
	}

	unsigned int LootingLevel = 0;
	if (a_Killer != nullptr)
	{
		LootingLevel = a_Killer->GetEquippedWeapon().m_Enchantments.GetLevel(cEnchantments::enchLooting);
	}
	AddRandomDropItem(a_Drops, 1, 3 + LootingLevel, IsOnFire() ? E_ITEM_COOKED_MUTTON : E_ITEM_RAW_MUTTON);
}





void cSheep::OnRightClicked(cPlayer & a_Player)
{
	super::OnRightClicked(a_Player);

	const cItem & EquippedItem = a_Player.GetEquippedItem();
	if ((EquippedItem.m_ItemType == E_ITEM_SHEARS) && !IsSheared() && !IsBaby())
	{
		m_IsSheared = true;
		m_World->BroadcastEntityMetadata(*this);
		a_Player.UseEquippedItem();

		cItems Drops;
		int NumDrops = m_World->GetTickRandomNumber(2) + 1;
		Drops.push_back(cItem(E_BLOCK_WOOL, static_cast<char>(NumDrops), static_cast<short>(m_WoolColor)));
		m_World->SpawnItemPickups(Drops, GetPosX(), GetPosY(), GetPosZ(), 10);
		m_World->BroadcastSoundEffect("mob.sheep.shear", GetPosX(), GetPosY(), GetPosZ(), 1.0f, 1.0f);
	}
	else if ((EquippedItem.m_ItemType == E_ITEM_DYE) && (m_WoolColor != 15 - EquippedItem.m_ItemDamage))
	{
		m_WoolColor = 15 - EquippedItem.m_ItemDamage;
		if (!a_Player.IsGameModeCreative())
		{
			a_Player.GetInventory().RemoveOneEquippedItem();
		}
		m_World->BroadcastEntityMetadata(*this);
	}
}





void cSheep::Tick(std::chrono::milliseconds a_Dt, cChunk & a_Chunk)
{
	super::Tick(a_Dt, a_Chunk);
	int PosX = POSX_TOINT;
	int PosY = POSY_TOINT - 1;
	int PosZ = POSZ_TOINT;

	if ((PosY <= 0) || (PosY >= cChunkDef::Height))
	{
		return;
	}

	if (m_TimeToStopEating > 0)
	{
		StopMovingToPosition();
		m_TimeToStopEating--;

		if (m_TimeToStopEating == 0)
		{
			if (m_World->GetBlock(PosX, PosY, PosZ) == E_BLOCK_GRASS)  // Make sure grass hasn't been destroyed in the meantime
			{
				// The sheep ate the grass so we change it to dirt
				m_World->SetBlock(PosX, PosY, PosZ, E_BLOCK_DIRT, 0);
				GetWorld()->BroadcastSoundParticleEffect(EffectID::PARTICLE_BLOCK_BREAK, PosX, PosY, PosX, E_BLOCK_GRASS);
				m_IsSheared = false;
				m_World->BroadcastEntityMetadata(*this);
			}
		}
	}
	else
	{
		if (m_World->GetTickRandomNumber(600) == 1)
		{
			if (m_World->GetBlock(PosX, PosY, PosZ) == E_BLOCK_GRASS)
			{
				m_World->BroadcastEntityStatus(*this, esSheepEating);
				m_TimeToStopEating = 40;
			}
		}
	}
}





NIBBLETYPE cSheep::GenerateNaturalRandomColor(void)
{
	cFastRandom Random;
	int Chance = Random.NextInt(101);

	if (Chance <= 81)
	{
		return E_META_WOOL_WHITE;
	}
	else if (Chance <= 86)
	{
		return E_META_WOOL_BLACK;
	}
	else if (Chance <= 91)
	{
		return E_META_WOOL_GRAY;
	}
	else if (Chance <= 96)
	{
		return E_META_WOOL_LIGHTGRAY;
	}
	else if (Chance <= 99)
	{
		return E_META_WOOL_BROWN;
	}
	else
	{
		return E_META_WOOL_PINK;
	}
}

