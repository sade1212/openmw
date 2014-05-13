
#include "aipackage.hpp"

#include <cmath>
#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"
#include "creaturestats.hpp"
#include "movement.hpp"

#include <OgreMath.h>

#include "steering.hpp"

MWMechanics::AiPackage::~AiPackage() {}


bool MWMechanics::AiPackage::pathTo(const MWWorld::Ptr& actor, ESM::Pathgrid::Point dest, float duration)
{
    //Update various Timers
    mTimer = mTimer + duration; //Update timer
    mStuckTimer = mStuckTimer + duration;   //Update stuck timer
    mTotalTime = mTotalTime + duration; //Update total time following

    ESM::Position pos = actor.getRefData().getPosition(); //position of the actor

    /// Stops the actor when it gets too close to a unloaded cell
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        const ESM::Cell *cell = actor.getCell()->getCell();
        Movement &movement = actor.getClass().getMovementSettings(actor);

        //Ensure pursuer doesn't leave loaded cells
        if(cell->mData.mX != player.getCell()->getCell()->mData.mX)
        {
            int sideX = PathFinder::sgn(cell->mData.mX - player.getCell()->getCell()->mData.mX);
            //check if actor is near the border of an inactive cell. If so, stop walking.
            if(sideX * (pos.pos[0] - cell->mData.mX*ESM::Land::REAL_SIZE) > sideX * (ESM::Land::REAL_SIZE/2.0f - 200.0f))
            {
                movement.mPosition[1] = 0;
                return false;
            }
        }
        if(cell->mData.mY != player.getCell()->getCell()->mData.mY)
        {
            int sideY = PathFinder::sgn(cell->mData.mY - player.getCell()->getCell()->mData.mY);
            //check if actor is near the border of an inactive cell. If so, stop walking.
            if(sideY * (pos.pos[1] - cell->mData.mY*ESM::Land::REAL_SIZE) > sideY * (ESM::Land::REAL_SIZE/2.0f - 200.0f))
            {
                movement.mPosition[1] = 0;
                return false;
            }
        }
    }

    //Start position
    ESM::Pathgrid::Point start = pos.pos;

    //***********************
    /// Checks if you can't get to the end position at all, adds end position to end of path
    /// Rebuilds path every quarter of a second, in case the target has moved
    //***********************
    if(mTimer > 0.25)
    {
        mPathFinder.buildPath(start, dest, actor.getCell(), true); //Rebuild path, in case the target has moved
        if(!mPathFinder.getPath().empty()) //Path has points in it
        {
            ESM::Pathgrid::Point lastPos = mPathFinder.getPath().back(); //Get the end of the proposed path

            if(distance(dest, lastPos) > 100) //End of the path is far from the destination
                mPathFinder.addPointToPath(dest); //Adds the final destination to the path, to try to get to where you want to go
        }

        mTimer = 0;
    }

    //************************
    /// Checks if you aren't moving; attempts to unstick you
    //************************
    if(mStuckTimer>0.5) //Checks every half of a second
    {
        if(distance(start, mStuckPos.pos[0], mStuckPos.pos[1], mStuckPos.pos[2]) < 10)  //NPC hasn't moved much is half a second, he's stuck
            mPathFinder.buildPath(start, dest, actor.getCell(), true);

        mStuckTimer = 0;
        mStuckPos = pos;
    }

    //Checks if the path isn't over, turn tomards the direction that you're going
    if(!mPathFinder.checkPathCompleted(pos.pos[0],pos.pos[1],pos.pos[2]))
    {
        zTurn(actor, Ogre::Degree(mPathFinder.getZAngleToNext(pos.pos[0], pos.pos[1])));
    }
}
