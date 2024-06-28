#include "InstructionsOrderManager.hpp"

void StmtOrder::moveInstruction(const Stmt* key1,const Stmt* key2)
{
    const Stmt* key,*newKey;
    
    if(key1==nullptr || key2==nullptr)
        return;
    //Choose the key with the biggest location (so further in the code), to avoid moving
    //an instruction behind a needed declaration
    if(key1->getBeginLoc()>key2->getBeginLoc())
    {
        key=key2;
        newKey=key1;
    }
    else
    {
        key=key1;
        newKey=key2;
    }

    if(instructionLinks.count(key)==0)
        return;
    if(instructionLinks.count(newKey)==0)
        return;
    auto groupNumberOld=instructionLinks.at(key);
    if(instructionGroups.count(groupNumberOld)==0)
        return;
    auto groupNumberNew=instructionLinks.at(newKey);
    if(instructionGroups.count(groupNumberNew)==0)
        return;

    auto& instructionGroup = instructionGroups.at(groupNumberOld);
    auto& instructionGroupNew = instructionGroups.at(groupNumberNew);
    for(auto instr:instructionGroup)
    {
        instructionGroupNew.insert(instr);
        instructionLinks.at(instr.first)=groupNumberNew;
    }
    instructionGroups.erase(groupNumberOld);
}
void fuseInstructions(const std::vector<const Stmt*> vect,StmtOrder& instructionsOrderManager)
{
    if(vect.size()<2)
        return;
    const Stmt* key=vect.at(0);
    for(auto instr:vect)
    {
        if(instr==key)
            continue;
        if(key->getBeginLoc()<instr->getBeginLoc())
        {
            key=instr;
        }
    }
    for(auto instr:vect)
    {
        if(instr==key)
            continue;
        instructionsOrderManager.moveInstruction(key,instr);
    }
}

std::string modifiedStringForInstruction(Rewriter& TheRewriter,const StmtOrder& instructionsOrderManager,const Stmt* instr)
{
    std::stringstream ssPrint;
    //If the instruction contains a group of instructions (for,if,...)
    StmtOrder* subOrder=instructionsOrderManager.getSubStmtOrder(instr).get();
    if(subOrder!=nullptr)
    {
        ssPrint<<getStmtAsString(instr,TheRewriter.getLangOpts())<<"{\n";
        for(auto instrSubGroups:subOrder->instructionGroups)
        {
            ssPrint<<"#pragma \n{\n";
            for(auto instrPair:instrSubGroups.second)
            {
                auto instr=instrPair.first;
                ssPrint<<modifiedStringForInstruction(TheRewriter,instructionsOrderManager,instr)<<";\n";
            }
            ssPrint<<"}\n";
        }
        ssPrint<<"}\n";
    }
    else
        ssPrint<<getStmtAsStringFull(instr,TheRewriter.getLangOpts())<<";\n";
    return ssPrint.str();
}

void modifyFile(Rewriter& TheRewriter,const StmtOrder& instructionsOrderManager)
{
    for(const auto& instruction : instructionsOrderManager.instructionGroups)
    {
        //Remove old text
        auto vect = instruction.second;

        const Stmt* st=(*vect.rbegin()).first;
        TheRewriter.RemoveText(SourceRange(st->getBeginLoc(),Lexer::getLocForEndOfToken(st->getEndLoc(),0,TheRewriter.getSourceMgr(),TheRewriter.getLangOpts())));
        if(vect.size()==0)
            continue;
        std::stringstream ssPrint;
        ssPrint<<"#pragma \n{\n";
        for(auto instrPair:vect)
        {
            auto instr=instrPair.first;
            TheRewriter.RemoveText(SourceRange(instr->getBeginLoc(),Lexer::getLocForEndOfToken(instr->getEndLoc(),0,TheRewriter.getSourceMgr(),TheRewriter.getLangOpts())));
            ssPrint<<modifiedStringForInstruction(TheRewriter,instructionsOrderManager,instr);   
        }
        ssPrint<<"}\n";
        TheRewriter.InsertText(st->getBeginLoc(),ssPrint.str());
    }
}