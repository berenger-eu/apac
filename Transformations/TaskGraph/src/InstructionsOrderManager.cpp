#include "InstructionsOrderManager.hpp"

void moveInstruction(const Stmt* key1,const Stmt* key2,instructionsOrder& instructionsOrderManager)
{
    const Stmt* key,*newKey;
    
    if(key1==nullptr || key2==nullptr)
        return;
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
    auto it = instructionsOrderManager.find(key);
    if(it!=instructionsOrderManager.end())
    {
        auto vectInstructions = it->second;
        if(instructionsOrderManager.find(newKey)==instructionsOrderManager.end())
        {
            return;
        }
        else
        {
    llvm::errs()<<"temp1\n";
            // instructionsOrderManager[newKey].push_back(key);
            for(int i=vectInstructions.size()-1;i>=0;i--)
            {
                    llvm::errs()<<"temp1\n"<<i<<"\n";

                instructionsOrderManager[newKey].push_front(vectInstructions[i]);
    llvm::errs()<<"temp2\n";
                /*
                if(vectInstructions[i]==key)
                {
                    instructionsOrderManager[key].erase(instructionsOrderManager[key].begin()+i);
                    break;
                }
                */
            }
                llvm::errs()<<"temp1\n";

        }
        vectInstructions.clear();
    }
}
void fuseInstructions(const std::vector<const Stmt*> vect,instructionsOrder& instructionsOrderManager)
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
        moveInstruction(key,instr,instructionsOrderManager);
    }
}

void modifyFile(Rewriter& TheRewriter,const instructionsOrder& instructionsOrderManager)
{
    for(const auto& instruction : instructionsOrderManager)
    {
        //Remove old text
        auto st=instruction.first;
        TheRewriter.RemoveText(SourceRange(st->getBeginLoc(),Lexer::getLocForEndOfToken(st->getEndLoc(),0,TheRewriter.getSourceMgr(),TheRewriter.getLangOpts())));
        auto vect = instruction.second;
        if(vect.size()==0)
            continue;
        std::stringstream ssPrint;
        ssPrint<<"#pragma \n{\n";
        for(auto instr:vect)
        {
            ssPrint<<getStmtAsString(instr,TheRewriter.getLangOpts())<<";\n";   
        }
        ssPrint<<"}\n";
        TheRewriter.InsertText(st->getBeginLoc(),ssPrint.str());
    }
}