
#include "Globals.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "Pawn.h"
#include "../World.h"
#include "../Bindings/PluginManager.h"
#include "BoundingBox.h"





cPawn::cPawn(eEntityType a_EntityType, double a_Width, double a_Height) :
	super(a_EntityType, 0, 0, 0, a_Width, a_Height)
	, m_EntityEffects(tEffectMap())
{
	SetGravity(-32.0f);
	SetAirDrag(0.02f);
}





void cPawn::Tick(std::chrono::milliseconds a_Dt, cChunk & a_Chunk)
{
	// Iterate through this entity's applied effects
	for (tEffectMap::iterator iter = m_EntityEffects.begin(); iter != m_EntityEffects.end();)
	{
		// Copies values to prevent pesky wrong accesses and erasures
		cEntityEffect::eType EffectType = iter->first;
		cEntityEffect * Effect = iter->second;
		
		Effect->OnTick(*this);
		
		// Iterates (must be called before any possible erasure)
		++iter;
		
		// Remove effect if duration has elapsed
		if (Effect->GetDuration() - Effect->GetTicks() <= 0)
		{
			RemoveEntityEffect(EffectType);
		}
		
		// TODO: Check for discrepancies between client and server effect values
	}

	class Pusher : public cEntityCallback
	{
	public:
		cEntity * m_Pusher;

		Pusher(cEntity * a_Pusher) :
			m_Pusher(a_Pusher)
		{
		}

		virtual bool Item(cEntity * a_Entity) override
		{
			if (a_Entity->GetUniqueID() == m_Pusher->GetUniqueID())
			{
				return false;
			}

			// we only push other mobs, boats and minecarts
			if ((a_Entity->GetEntityType() != etMonster) && (a_Entity->GetEntityType() != etMinecart) && (a_Entity->GetEntityType() != etBoat))
			{
				return false;
			}

			Vector3d v3Delta = a_Entity->GetPosition() - m_Pusher->GetPosition();
			v3Delta.y = 0.0;  // we only push sideways
			v3Delta *= 1.0 / (v3Delta.Length() + 0.01);  // we push harder if we're close
			// QUESTION: is there an additional multiplier for this? current shoving seems a bit weak

			a_Entity->AddSpeed(v3Delta);
			return false;
		}
	} Callback(this);

	m_World->ForEachEntityInBox(cBoundingBox(GetPosition(), GetWidth(), GetHeight()), Callback);
	
	super::Tick(a_Dt, a_Chunk);
}





void cPawn::KilledBy(TakeDamageInfo & a_TDI)
{
	ClearEntityEffects();
	super::KilledBy(a_TDI);
}





bool cPawn::IsFireproof(void) const
{
	return super::IsFireproof() || HasEntityEffect(cEntityEffect::effFireResistance);
}





void cPawn::HandleAir(void)
{
	if (IsSubmerged() && HasEntityEffect(cEntityEffect::effWaterBreathing))
	{
		// Prevent the oxygen from decreasing
		return;
	}
	
	super::HandleAir();
}





void cPawn::AddEntityEffect(cEntityEffect::eType a_EffectType, int a_Duration, short a_Intensity, double a_DistanceModifier)
{
	// Check if the plugins allow the addition:
	if (cPluginManager::Get()->CallHookEntityAddEffect(*this, a_EffectType, a_Duration, a_Intensity, a_DistanceModifier))
	{
		// A plugin disallows the addition, bail out.
		return;
	}
	
	// No need to add empty effects:
	if (a_EffectType == cEntityEffect::effNoEffect)
	{
		return;
	}
	a_Duration = static_cast<int>(a_Duration * a_DistanceModifier);
	
	m_EntityEffects[a_EffectType] = cEntityEffect::CreateEntityEffect(a_EffectType, a_Duration, a_Intensity, a_DistanceModifier);
	m_World->BroadcastEntityEffect(*this, a_EffectType, a_Intensity, static_cast<short>(a_Duration));
	m_EntityEffects[a_EffectType]->OnActivate(*this);
}





void cPawn::RemoveEntityEffect(cEntityEffect::eType a_EffectType)
{
	m_World->BroadcastRemoveEntityEffect(*this, a_EffectType);
	m_EntityEffects[a_EffectType]->OnDeactivate(*this);
	delete m_EntityEffects[a_EffectType];
	m_EntityEffects.erase(a_EffectType);
}





bool cPawn::HasEntityEffect(cEntityEffect::eType a_EffectType) const
{
	return m_EntityEffects.find(a_EffectType) != m_EntityEffects.end();
}





void cPawn::ClearEntityEffects()
{
	// Iterate through this entity's applied effects
	for (tEffectMap::iterator iter = m_EntityEffects.begin(); iter != m_EntityEffects.end();)
	{
		// Copy values to prevent pesky wrong erasures
		cEntityEffect::eType EffectType = iter->first;
		
		// Iterates (must be called before any possible erasure)
		++iter;
		
		// Remove effect
		RemoveEntityEffect(EffectType);
	}
}



