//------------------------------------------------------------------------------
// Tooling sample. Demonstrates:
//
// * How to write a simple source tool using libTooling.
// * How to use RecursiveASTVisitor to find interesting AST nodes.
// * How to use the Rewriter API to rewrite the source code.
//
// Eli Bendersky (eliben@gmail.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#include <sstream>
#include <string>

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Lex/Lexer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;

vector<IfStmt*> ifstmtvector;
vector<FunctionDecl> functiondeclvector;
vector<Stmt*> stmtvector;

vector<forstmt*> forstmtvector;

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");


//get the source code of the specific parts of AST
template <typename T>
static std::string getText(const SourceManager &SourceManager, const T &Node) {
  SourceLocation StartSpellingLocation =
      SourceManager.getSpellingLoc(Node.getLocStart());
  SourceLocation EndSpellingLocation =
      SourceManager.getSpellingLoc(Node.getLocEnd());
  if (!StartSpellingLocation.isValid() || !EndSpellingLocation.isValid()) {
    return std::string();
  }
  bool Invalid = true;
  const char *Text =
      SourceManager.getCharacterData(StartSpellingLocation, &Invalid);
  if (Invalid) {
    return std::string();
  }
  std::pair<FileID, unsigned> Start =
      SourceManager.getDecomposedLoc(StartSpellingLocation);
  std::pair<FileID, unsigned> End =
      SourceManager.getDecomposedLoc(Lexer::getLocForEndOfToken(
          EndSpellingLocation, 0, SourceManager, LangOptions()));
  if (Start.first != End.first) {
    // Start and end are in different files.
    return std::string();
  }
  if (End.second < Start.second) {
    // Shuffling text with macros may cause this.
    return std::string();
  }
  return std::string(Text, End.second - Start.second);
}

// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.
class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
public:
  MyASTVisitor(Rewriter &R) : TheRewriter(R) {}

  bool VisitStmt(Stmt *s) {
    // Collect If statements.
    if (isa<IfStmt>(s)) {
      ifstmtvector.push_back(cast<IfStmt>(s));
    }
    //collect for statements
    if(isa<forstmt>(s)){
	forstmtvector.push_back(cast<forstmt>(s))
    }
    //Collect other statements
    else{
      stmtvector.push_back(s);
    }
    return true;
  }

  bool VisitFunctionDecl(FunctionDecl *f) {
    // Only function definitions (with bodies), not declarations.
    if (f->hasBody()) {
      functiondeclvector.push_back(*f);
    }

    return true;
  }

private:
  Rewriter &TheRewriter;
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R) : Visitor(R) {}

  // Override the method that gets called for each parsed top-level
  // declaration.
  bool HandleTopLevelDecl(DeclGroupRef DR) override {
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
      // Traverse the declaration using our AST visitor.
      Visitor.TraverseDecl(*b);
      //(*b)->dump();
    }
    return true;
  }

private:
  MyASTVisitor Visitor;
};

// For each source file provided to the tool, a new FrontendAction is created.
class MyFrontendAction : public ASTFrontendAction {
public:
  MyFrontendAction() {}
  void EndSourceFileAction() override {
    SourceManager &SM = TheRewriter.getSourceMgr();
    llvm::errs() << "** EndSourceFileAction for: "
                 << SM.getFileEntryForID(SM.getMainFileID())->getName() << "\n";


    //handle each function definition
    for(unsigned funcid = 0; funcid < functiondeclvector.size(); funcid++){
      FunctionDecl *f = &functiondeclvector[funcid];
      Stmt *FuncBody = f->getBody();

      // Function name
      DeclarationName DeclName = f->getNameInfo().getName();
      std::string FuncName = DeclName.getAsString();

      FullSourceLoc funcstart = Context->getFullLoc(FuncBody->getLocStart());
      FullSourceLoc funcend = Context->getFullLoc(FuncBody->getLocEnd());
      unsigned startln = funcstart.getSpellingLineNumber();
      unsigned endln = funcend.getSpellingLineNumber();
      ofstream myfile;
      myfile.open (FuncName + ".dot");
      cout<< (FuncName + ".dot")<<endl;
      myfile << "digraph unnamed {\n";
      

      if(ifstmtvector.size() != 1) {
        cout<<"one if statement only" << endl;
        continue;
      }

      if(forstmtvector.size() !=1){
	cout<<"one for statement only" <<endl;
	continue;
      }
      myfile << "Node1 [shape=record,label=\"{ [(ENTRY)]\\l}\"];\n";
      

      string nodebeforeif = "";
      // Here we only handle variable definition. 
      // To make it fully functional, other statment has to be handled too.
      for(unsigned stmtid = 0; stmtid < stmtvector.size(); stmtid++){
        /*
        if(isa<BinaryOperator>(stmtvector[stmtid])) {
          BinaryOperator *b = cast<BinaryOperator>(stmtvector[stmtid]);
          if(b->isAssignmentOp()){
            cout<<getText(SM, *b)<<endl;
          }
        }*/
        if(isa<DeclStmt>(stmtvector[stmtid])) {       
          DeclStmt *ds = cast<DeclStmt>(stmtvector[stmtid]);
          VarDecl *vd = cast<VarDecl>(ds->getSingleDecl());
          if(vd){
            nodebeforeif = nodebeforeif + getText(TheRewriter.getSourceMgr(), *vd) + "\\l";
          }
        } 
      }
      myfile << "Node2 [shape=record,label=\"" + nodebeforeif + "\"]\n";
      myfile << "Node1 -> Node2;\n";
      //We assume one if statement here
     
      IfStmt *IfStatement = ifstmtvector[0];
      forstmt *ForStatement = forstmtvector[0];
      
      string condition = getText(TheRewriter.getSourceMgr(), *IfStatement->getCond());
      myfile << "Node3 [shape=record,label=\"" + condition + "\"]\n";
      myfile << "Node2 -> Node3;\n";
      Stmt *Then = IfStatement->getThen();
      string thenpart = getText(SM, *Then);
      myfile << "Node4 [shape=record,label=\"" + thenpart + "\"]\n";
      myfile << "Node3 -> Node4;\n";
      myfile << "Node6 [shape=record,label=\"{ [(Exit)]\\l}\"];\n";
      myfile << "Node4 -> Node6;\n";
      Stmt *Else = IfStatement->getElse();
      if (Else){
        string elsepart = getText(SM, *Else);
        myfile << "Node5 [shape=record,label=\"" + elsepart + "\"]\n";
        myfile << "Node3 -> Node5;\n";
        myfile << "Node5 -> Node6;\n";
      }
      
      myfile << "}\n";
      myfile.close();

    }
  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {
    llvm::errs() << "** Creating AST consumer for: " << file << "\n";
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    Context = &CI.getASTContext();
    return llvm::make_unique<MyASTConsumer>(TheRewriter);
  }

private:
  Rewriter TheRewriter;
  ASTContext *Context;
};

int main(int argc, const char **argv) {
  CommonOptionsParser op(argc, argv, ToolingSampleCategory);
  ClangTool Tool(op.getCompilations(), op.getSourcePathList());

  // ClangTool::run accepts a FrontendActionFactory, which is then used to
  // create new objects implementing the FrontendAction interface. Here we use
  // the helper newFrontendActionFactory to create a default factory that will
  // return a new MyFrontendAction object every time.
  // To further customize this, we could create our own factory class.
  return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
