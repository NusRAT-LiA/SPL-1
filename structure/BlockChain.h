#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H
#include <unistd.h>

#include <list>
#include "Block.h"
#include "Transaction.h"
#include "Wallet.h"
#include "Miner.h"
using namespace std;


class Blockchain {

    

public:
    int difficulty;
    vector<Block> blocks;
    Block genesisBlock;
    vector<Transaction> pendingTransactions;
    vector<Transaction> addedTransactions;
    list<Wallet> Wallets;
    map<pair<long long int , long long int> , Wallet > KeyMap;
    map<int,Miner> minerMap;
    Miner defaultMiner ;

    string chainName;

    Blockchain(){}
    Blockchain(int difficulty, string Name)
    {
        this->difficulty=difficulty;
        this->chainName=Name;
    }
    

    Block getBlock(int index)
    { 
      if(this->blocks.size()<index+1)
       throw out_of_range("");

      return this->blocks[index];
    }



    Wallet addWallet(string Name , int balance)
    {
        Wallet NewWallet=Wallet(Name,balance);
        this->KeyMap[make_pair( NewWallet.WalletOwner.PublicKey.getX(),NewWallet.WalletOwner.PublicKey.getY())]=NewWallet;
        this->Wallets.push_back(NewWallet);
        return NewWallet;
    }
    

    void addMiner(int minerId)
    {
      Miner newMiner(minerId);
      cout<<"Miner ID "<<newMiner.getId()<<" created"<<endl;
      this->minerMap[minerId]=newMiner;
    }


    Miner getMiner(int ID)
    {   
        return this->minerMap.at(ID);
    }



    void addTransaction(pair<long long int,long long int> SenderKey , pair<long long int,long long int> RecieverKey , int Amount, long long int Sign,int offeredFee)
    {
        Transaction Tx = Transaction(SenderKey,RecieverKey,Amount,Sign,offeredFee);
        cout<<"Transaction hash : "<<Tx.TxHash<<endl;sleep(1);
        if(this->pendingTransactions.size()>1 && this->pendingTransactions.back().TxFee>Tx.TxFee)
        {
            Transaction prevTx = this->pendingTransactions.back();
            this->pendingTransactions.pop_back();
            this->pendingTransactions.push_back(Tx);
            this->pendingTransactions.push_back(prevTx);
        }
        else
        this->pendingTransactions.push_back(Tx);
        
        //sort(this->pendingTransactions.begin(),this->pendingTransactions.end(),cmp);
    }



    void mineBlock(Miner miner)
    { 
      if(this->pendingTransactions.size()==0)
      {
        cout<<"No transaction is pending on the chain "<<endl;sleep(1);
        cout<<"Block Creation unsuccessful !\n"<<endl;
        return;
      }
      vector<Transaction> CollecttedTx;
      int numOfCollectedTx=3;

      cout<<"\nMiner ID-"<<miner.getId()<<" Collecting Transactions from the network with higher fees "<<endl;sleep(2);

      
      while(numOfCollectedTx--)
      { 
        if(this->pendingTransactions.size()==0)break;
        Transaction tx = this->pendingTransactions.back();
        CollecttedTx.push_back(tx);
        this->pendingTransactions.pop_back();
      }
      cout<<"Block being created with .."<<endl;sleep(2);
      for(auto i :CollecttedTx)
      {
      cout<<"Tx hash            : "<<i.TxHash<<endl;sleep(1);
      }
      cout<<"\n";
      cout<<"Network difficulty : "<<this->difficulty<<endl;sleep(2);
      cout<<"Previous blockhash : "<<this->blocks.back().hash<<endl;sleep(2);

      Block Newblock = miner.createBlock(CollecttedTx,this->blocks.back().hash,this->difficulty);
      Newblock.index=this->blocks.size();
      
      cout<<"\nMiner ID-"<<miner.getId()<<"  started verifying block's transactions..."<<endl;sleep(2);
      Newblock = miner.verifyTransactions(Newblock,this->KeyMap);
      cout<<"\nMiner ID"<<miner.getId()<<" started mining block"<<endl;sleep(2);
      if(Newblock.transactions.size()==0)
      {
        cout<<"Block has no verified Transactions ! Trying mining again !"<<endl;
        this->mineBlock(miner);
        return ;
      }
      Newblock=miner.mineBlock(Newblock);
    
      cout<<"Block mined with nonce "<<Newblock.nonce<<endl;sleep(1);

      this->blocks.push_back(Newblock);
      
      for(auto i :Newblock.transactions)
      { 
        this->addedTransactions.push_back(i);
        cout<<"Transaction "<<i.TxHash<<"successful !"<<endl;
        this->KeyMap[i.PublicKeyOfSenderWallet].removeBalance(i.AmountSent+i.TxFee);
        cout<<"New balance of sender wallet"<<this->KeyMap[i.PublicKeyOfSenderWallet].WalletAdress<<" : "<<this->KeyMap[i.PublicKeyOfSenderWallet].getBalance()<<endl;
        this->KeyMap[i.PublicKeyOfRecieverWallet].addBalance(i.AmountSent);
        cout<<"New balance of reciever wallet"<<this->KeyMap[i.PublicKeyOfRecieverWallet].WalletAdress<<" : "<<this->KeyMap[i.PublicKeyOfRecieverWallet].getBalance()<<endl;
        miner.minerBalance+=i.TxFee;
        cout<<"Transaction fee "<<i.TxFee<<" added to Miner ID "<<miner.getId()<<"'s balance"<<endl;
        cout<<"\n";
      }

      cout<<"Block added to the chain successfully !"<<endl;    

    }





    void createGenesisBlock()
    {   
        cout<<"\n**Creating GenesisBlock(The first block of a blockchain).....**\n"<<endl;sleep(2);
        cout<<"\n**Adding a null Transaction...**\n"<<endl;sleep(2);
        this->addTransaction(pair<long long , long long >(),pair<long long ,long  long >(),0,0,0);
        
        cout<<"\n**Adding Default Miner in the chain with MinerId-0...**\n"<<endl;sleep(2);
        this->defaultMiner=Miner(0);
        this->minerMap[0]=defaultMiner;
        cout<<"**DefaultMiner adding null Transaction to a block...**\n"<<endl;sleep(1);
        Block Newblock = defaultMiner.createBlock(pendingTransactions,"0",this->difficulty);
        this->pendingTransactions.clear();
        Newblock.index=this->blocks.size();
    
        this->genesisBlock=defaultMiner.mineBlock(Newblock);
        this->blocks.push_back(this->genesisBlock);
        cout<<"**DefaultMiner Mined the Genesis Block...**\n"<<endl;

    }

    
    void setBlock(int  blockIndex , Block newBlock , int TxIndex , Transaction Tx)
    {
       newBlock.setTransaction(TxIndex,Tx);
       this->blocks[blockIndex]=newBlock;
       if(this->blocks.size()==1)return;

       int i=blockIndex+1;
       while (i<this->blocks.size())
       {  
          this->blocks[i].previousHash=this->blocks[i-1].hash;
          this->blocks[i].hash=this->blocks[i].calculateHash();
          cout<<"Block-"<<i<<" 's hash changed to -> "<<this->blocks[i].hash<<endl;sleep(2);

          i++;
       }
       
    }
    
     
    
};

#endif /* BLOCKCHAIN_H */
