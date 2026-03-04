// =================== DIGITAL BANK V3 - FULL FINAL (PURE C, FIXED) ===================
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <conio.h>
#else
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#endif

#define ADMIN_PIN 9999
#define DATA_DIR "ALL_RECORDS/"
#define MIN_BALANCE 1000.0
#define PENALTY_FEE 50.0

typedef struct {
    int accNo;
    char name[50];
    char email[50];
    char phone[15];
    int pin;
    float balance;
    int depositCount;
    int withdrawCount;
} User;

// ================= PROTOTYPES (FIX for implicit declaration errors) =================
void ensureDataDir();
void inputHidden(char *buf, int maxLen);
void userFile(char *o, int a);
void historyFile(char *o, int a);
void inboxFile(char *o, int a);
void karmaFile(char *o, int a);
void addToIndex(int accNo);
int  isBlocked(int accNo);
void blockAccount(int accNo);
void unblockAccount(int accNo);
void logAdminAction(const char *action);
int  isGlobalFreeze();
void toggleGlobalFreeze(int on);
void saveUser(strcat User u);
int  loadUser(int accNo, User *u);
void logTxn(int accNo, const char *type, float amt, int other);
void pushInbox(int accNo, const char *msg);
void showInbox(int accNo);
void adminFraudRadar();
void adminBroadcast();
int  generateOTP();
int  verifyOTP(const char *type);
void generateMonthlyEmail(int accNo);
void changePIN(User *u);
int  loadKarma(int accNo);
void saveKarma(int accNo, int k);
void showKarma(int accNo);
void applyMinBalanceRule(User *u);
void financialMirror(int accNo);
void explainLastTransaction(int accNo);
void voiceCommand(User *u, const char *cmd);
void smartChatbot(User *u);
void showHistory(int accNo);
void transferMoney(User *u);
void bankMenu(User *u);
void createAccount();
void login();
void adminPanel();

// ---------------- DIR ----------------
void ensureDataDir() {
#ifdef _WIN32
    _mkdir(DATA_DIR);
#else
    mkdir(DATA_DIR, 0777);
#endif
}

// ---------------- HIDDEN INPUT ----------------
void inputHidden(char *buf, int maxLen) {
    int i = 0; char ch;
#ifdef _WIN32
    while (1) {
        ch = _getch();
        if (ch == '\r') break;
        if (ch == '\b' && i > 0) { i--; printf("\b \b"); }
        else if (i < maxLen - 1 && ch >= '0' && ch <= '9') { buf[i++] = ch; printf("*"); }
    }
#else
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt; newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    while ((ch = getchar()) != '\n' && i < maxLen - 1) { buf[i++] = ch; printf("*"); fflush(stdout); }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
    buf[i] = '\0'; printf("\n");
}

// ---------------- FILE HELPERS ----------------
void userFile(char *o, int a){ sprintf(o, DATA_DIR "user_%d.txt", a); }
void historyFile(char *o, int a){ sprintf(o, DATA_DIR "history_%d.txt", a); }
void inboxFile(char *o, int a){ sprintf(o, DATA_DIR "inbox_%d.txt", a); }
void karmaFile(char *o, int a){ sprintf(o, DATA_DIR "karma_%d.txt", a); }

// ---------------- INDEX / BLOCK ----------------
void addToIndex(int accNo){ FILE *fp=fopen(DATA_DIR "accounts_index.txt","a"); if(fp){fprintf(fp,"%d\n",accNo); fclose(fp);} }
int isBlocked(int accNo){
    FILE *fp=fopen(DATA_DIR "blocked_accounts.txt","r"); int x; if(!fp) return 0;
    while(fscanf(fp,"%d",&x)!=EOF) if(x==accNo){ fclose(fp); return 1; }
    fclose(fp); return 0;
}
void blockAccount(int accNo){ if(isBlocked(accNo)) return; FILE *fp=fopen(DATA_DIR "blocked_accounts.txt","a"); if(fp){fprintf(fp,"%d\n",accNo); fclose(fp);} }
void unblockAccount(int accNo){
    FILE *fp=fopen(DATA_DIR "blocked_accounts.txt","r");
    FILE *tp=fopen(DATA_DIR "temp.txt","w"); int x; if(!fp||!tp) return;
    while(fscanf(fp,"%d",&x)!=EOF) if(x!=accNo) fprintf(tp,"%d\n",x);
    fclose(fp); fclose(tp); remove(DATA_DIR "blocked_accounts.txt"); rename(DATA_DIR "temp.txt", DATA_DIR "blocked_accounts.txt");
}

// ---------------- ADMIN AUDIT ----------------
void logAdminAction(const char *action){
    FILE *fp=fopen(DATA_DIR "admin_audit.log","a");
    time_t t=time(NULL); struct tm *tm=localtime(&t);
    fprintf(fp,"%02d-%02d-%04d %02d:%02d:%02d | %s\n",
            tm->tm_mday,tm->tm_mon+1,tm->tm_year+1900,tm->tm_hour,tm->tm_min,tm->tm_sec,action);
    fclose(fp);
}

// ---------------- GLOBAL FREEZE ----------------
int isGlobalFreeze(){
    FILE *fp=fopen(DATA_DIR "GLOBAL_FREEZE.flag","r");
    if(fp){ fclose(fp); return 1; }
    return 0;
}
void toggleGlobalFreeze(int on){
    if(on){
        FILE *fp=fopen(DATA_DIR "GLOBAL_FREEZE.flag","w"); if(fp) fclose(fp);
        printf("🧊 Global freeze ENABLED.\n"); logAdminAction("Global freeze enabled");
    } else {
        remove(DATA_DIR "GLOBAL_FREEZE.flag");
        printf("🔥 Global freeze DISABLED.\n"); logAdminAction("Global freeze disabled");
    }
}

// ---------------- USER I/O ----------------
void saveUser(User u){
    char f[120]; userFile(f,u.accNo);
    FILE *fp=fopen(f,"w");
    fprintf(fp,"%d\n%s\n%s\n%s\n%d\n%.2f\n%d\n%d", u.accNo,u.name,u.email,u.phone,u.pin,u.balance,u.depositCount,u.withdrawCount);
    fclose(fp);
}
int loadUser(int accNo, User *u){
    char f[120]; userFile(f,accNo);
    FILE *fp=fopen(f,"r"); if(!fp) return 0;
    fscanf(fp,"%d\n",&u->accNo);
    fgets(u->name,50,fp); u->name[strcspn(u->name,"\n")]=0;
    fgets(u->email,50,fp); u->email[strcspn(u->email,"\n")]=0;
    fgets(u->phone,15,fp); u->phone[strcspn(u->phone,"\n")]=0;
    fscanf(fp,"%d\n%f\n%d\n%d",&u->pin,&u->balance,&u->depositCount,&u->withdrawCount);
    fclose(fp); return 1;
}

// ---------------- LOG / INBOX ----------------
void logTxn(int accNo, const char *type, float amt, int other){
    char f[120]; historyFile(f,accNo);
    FILE *fp=fopen(f,"a"); time_t t=time(NULL); struct tm *tm=localtime(&t);
    fprintf(fp,"%02d-%02d-%04d %02d:%02d:%02d | %s | %.2f | with %d\n",
            tm->tm_mday,tm->tm_mon+1,tm->tm_year+1900,tm->tm_hour,tm->tm_min,tm->tm_sec,type,amt,other);
    fclose(fp);
}
void pushInbox(int accNo, const char *msg){
    char f[120]; inboxFile(f,accNo);
    FILE *fp=fopen(f,"a"); time_t t=time(NULL); struct tm *tm=localtime(&t);
    fprintf(fp,"%02d-%02d-%04d %02d:%02d:%02d | %s\n",
            tm->tm_mday,tm->tm_mon+1,tm->tm_year+1900,tm->tm_hour,tm->tm_min,tm->tm_sec,msg);
    fclose(fp);
}
void showInbox(int accNo){
    char f[120], line[300]; inboxFile(f,accNo);
    FILE *fp=fopen(f,"r"); printf("\n--- 📬 INBOX ---\n");
    if(!fp){ printf("No messages yet.\n"); return; }
    while(fgets(line,sizeof(line),fp)) printf("%s",line);
    fclose(fp);
}

// ---------------- FRAUD RADAR ----------------
void adminFraudRadar(){
    FILE *fp=fopen(DATA_DIR "accounts_index.txt","r");
    int acc; User u;
    printf("\n🕵️ FRAUD RADAR (Heuristic)\n");
    if(!fp){ printf("No users.\n"); return; }
    while(fscanf(fp,"%d",&acc)!=EOF){
        if(loadUser(acc,&u)){
            if(u.withdrawCount > u.depositCount + 5)
                printf("⚠️ Acc %d (%s): High withdrawal pattern\n", u.accNo, u.name);
        }
    }
    fclose(fp);
}

// ---------------- BROADCAST ----------------
void adminBroadcast(){
    char msg[200]; int acc;
    printf("Broadcast message: "); getchar(); fgets(msg,200,stdin);
    msg[strcspn(msg,"\n")] = 0;

    FILE *fp=fopen(DATA_DIR "accounts_index.txt","r");
    if(!fp){ printf("No users.\n"); return; }
    while(fscanf(fp,"%d",&acc)!=EOF) pushInbox(acc, msg);
    fclose(fp);
    printf("📢 Broadcast sent to all users.\n");
    logAdminAction("Broadcast sent");
}

// ---------------- OTP ----------------
int generateOTP(){ return rand()%9000 + 1000; }
int verifyOTP(const char *type){
    int otp=generateOTP(), userOTP;
    printf("📩 OTP sent to your %s: %d (simulation)\n", type, otp);
    printf("Enter OTP: "); scanf("%d",&userOTP);
    return otp==userOTP;
}

// ---------------- MONTHLY EMAIL ----------------
void generateMonthlyEmail(int accNo){
    char hist[120], out[150], line[300]; historyFile(hist, accNo);
    time_t t=time(NULL); struct tm *tm=localtime(&t);
    sprintf(out, DATA_DIR "email_%d_%04d_%02d.txt", accNo, tm->tm_year+1900, tm->tm_mon+1);
    FILE *in=fopen(hist,"r"), *outf=fopen(out,"w"); if(!in||!outf){ if(in)fclose(in); if(outf)fclose(outf); return; }
    fprintf(outf,"Monthly Statement for %d (%02d-%04d)\n\n", accNo, tm->tm_mon+1, tm->tm_year+1900);
    while(fgets(line,sizeof(line),in)) fputs(line,outf);
    fclose(in); fclose(outf);
    pushInbox(accNo,"📧 Monthly statement sent (simulated email).");
}

// ---------------- CHANGE PIN ----------------
void changePIN(User *u){
    char oldStr[10], newStr[10], confStr[10];
    printf("Old PIN: "); inputHidden(oldStr,sizeof(oldStr));
    if(atoi(oldStr)!=u->pin){ printf("❌ Wrong old PIN.\n"); return; }
    printf("New 4-digit PIN: "); inputHidden(newStr,sizeof(newStr));
    printf("Confirm PIN: "); inputHidden(confStr,sizeof(confStr));
    if(strcmp(newStr,confStr)!=0 || strlen(newStr)!=4){ printf("❌ PIN mismatch/invalid.\n"); return; }
    u->pin = atoi(newStr); saveUser(*u);
    pushInbox(u->accNo,"Your PIN was changed successfully.");
    printf("✅ PIN changed.\n");
}

// ---------------- KARMA ----------------
int loadKarma(int accNo){ char f[120]; int k=0; karmaFile(f,accNo); FILE *fp=fopen(f,"r"); if(fp){fscanf(fp,"%d",&k); fclose(fp);} return k; }
void saveKarma(int accNo, int k){ char f[120]; karmaFile(f,accNo); FILE *fp=fopen(f,"w"); fprintf(fp,"%d",k); fclose(fp); }
void showKarma(int accNo){
    int k=loadKarma(accNo);
    printf("\n🧿 Financial Karma: %d\n",k);
    if(k>10) printf("Good Saver Energy 😇\n");
    else if(k<0) printf("Spending Chaos 😈\n");
    else printf("Neutral 🙂\n");
}

// ---------------- MIN BALANCE ----------------
void applyMinBalanceRule(User *u){
    if(u->balance < MIN_BALANCE){
        u->balance -= PENALTY_FEE;
        saveUser(*u);
        pushInbox(u->accNo,"⚠️ Minimum balance not maintained. ₹50 penalty applied.");
        printf("⚠️ Min balance violated. ₹50 penalty charged.\n");
    }
}

// ---------------- MIRROR ----------------
void financialMirror(int accNo){
    char f[120], line[300], type[40]; int d,m,y; float amt; time_t now=time(NULL); float dep=0,wit=0;
    historyFile(f,accNo); FILE *fp=fopen(f,"r"); if(!fp){ printf("No history yet.\n"); return; }
    while(fgets(line,sizeof(line),fp)){
        if(sscanf(line,"%d-%d-%d %*[^|]| %39[^|]| %f",&d,&m,&y,type,&amt)==5){
            struct tm tx={0}; tx.tm_mday=d; tx.tm_mon=m-1; tx.tm_year=y-1900;
            double days=difftime(now,mktime(&tx))/(60*60*24);
            if(days<=7 && days>=0){
                if(strcmp(type,"DEPOSIT")==0) dep+=amt;
                if(strcmp(type,"WITHDRAW")==0 || strcmp(type,"TRANSFER_OUT")==0) wit+=amt;
            }
        }
    }
    fclose(fp);
    printf("\n🪞 Financial Mirror (Last 7 days)\nDeposited: ₹%.2f\nWithdrew: ₹%.2f\n",dep,wit);
    if(wit>dep) printf("Mirror: You're overspending 😬\n");
    else if(dep>wit) printf("Mirror: Good saving habit 😄\n");
    else printf("Mirror: Balanced 🙂\n");
}

// ---------------- EXPLAIN LAST ----------------
void explainLastTransaction(int accNo){
    char f[120], line[300], last[300]=""; historyFile(f,accNo);
    FILE *fp=fopen(f,"r"); if(!fp){ printf("No transactions.\n"); return; }
    while(fgets(line,sizeof(line),fp)) strcpy(last,line);
    fclose(fp);
    if(strlen(last)==0){ printf("No transactions.\n"); return; }
    printf("\n🧩 Last Transaction:\n%s", last);
    printf("Reason: Based on your recent activity pattern.\n");
}

// ---------------- CHATBOT + VOICE ----------------
void voiceCommand(User *u, const char *cmd){
    if(strcmp(cmd,"balance")==0) printf("🎤 Voice Bot: Balance ₹%.2f\n",u->balance);
    else if(strcmp(cmd,"history")==0) showHistory(u->accNo);
    else if(strcmp(cmd,"tips")==0){
        if(u->balance<500) printf("🎤 Voice Bot: Save small amounts daily.\n");
        else printf("🎤 Voice Bot: Keep saving!\n");
    } else printf("🎤 Voice Bot: Unknown command.\n");
}
void smartChatbot(User *u){
    char cmd[80];
    printf("\n🤖 Smart Assistant (help, balance, summary, tips, spend, email, history, karma, mirror, explain, exit)\n");
    getchar();
    while(1){
        printf("You: ");
        fgets(cmd,sizeof(cmd),stdin);
        cmd[strcspn(cmd,"\n")] = 0;

        if(strncmp(cmd,"voice: ",7)==0){ voiceCommand(u, cmd+7); continue; }
        if(strcmp(cmd,"help")==0) printf("Bot: help, balance, summary, tips, spend, email, history, karma, mirror, explain, exit\n");
        else if(strcmp(cmd,"balance")==0) printf("Bot: Balance ₹%.2f\n",u->balance);
        else if(strcmp(cmd,"summary")==0) printf("Bot: Deposits=%d Withdrawals=%d Balance=₹%.2f\n",u->depositCount,u->withdrawCount,u->balance);
        else if(strcmp(cmd,"tips")==0){
            if(u->withdrawCount>u->depositCount) printf("Bot: Spending more than saving. Set a budget.\n");
            else printf("Bot: Good saving trend.\n");
        } else if(strcmp(cmd,"spend")==0) printf("Bot: Withdrawals=%d Deposits=%d\n",u->withdrawCount,u->depositCount);
        else if(strcmp(cmd,"email")==0){ generateMonthlyEmail(u->accNo); printf("Bot: Monthly statement sent.\n"); }
        else if(strcmp(cmd,"history")==0) showHistory(u->accNo);
        else if(strcmp(cmd,"karma")==0) showKarma(u->accNo);
        else if(strcmp(cmd,"mirror")==0) financialMirror(u->accNo);
        else if(strcmp(cmd,"explain")==0) explainLastTransaction(u->accNo);
        else if(strcmp(cmd,"exit")==0){ printf("Bot: Bye! 💙\n"); break; }
        else printf("Bot: Unknown. Type help.\n");
    }
}

// ---------------- CORE ----------------
void showHistory(int accNo){
    char f[120], line[300]; historyFile(f,accNo);
    FILE *fp=fopen(f,"r"); printf("\n--- HISTORY ---\n");
    if(!fp){ printf("No transactions.\n"); return; }
    while(fgets(line,sizeof(line),fp)) printf("%s",line);
    fclose(fp);
}
void transferMoney(User *u){
    int toAcc; float amt; User r;
    printf("Receiver AccNo: "); scanf("%d",&toAcc);
    if(isGlobalFreeze()){ printf("🧊 System under maintenance.\n"); return; }
    if(isBlocked(u->accNo)||isBlocked(toAcc)){ printf("❌ Blocked.\n"); return; }
    if(!loadUser(toAcc,&r)){ printf("❌ Receiver not found.\n"); return; }
    printf("Amount: "); scanf("%f",&amt);
    if(amt<=0){ printf("❌ Invalid amount.\n"); return; }
    if(amt>u->balance){ printf("❌ Insufficient funds.\n"); return; }
    u->balance-=amt; r.balance+=amt; u->withdrawCount++;
    saveUser(*u); saveUser(r);
    logTxn(u->accNo,"TRANSFER_OUT",amt,toAcc);
    logTxn(toAcc,"TRANSFER_IN",amt,u->accNo);
    saveKarma(u->accNo, loadKarma(u->accNo)-2);
    applyMinBalanceRule(u);
    pushInbox(u->accNo,"Transfer completed.");
    printf("💸 Transfer successful!\n");
}
void bankMenu(User *u){
    int ch; float amt;
    do{
        printf("\nWelcome %s (%d)\n",u->name,u->accNo);
        printf("1.Balance 2.Deposit 3.Withdraw 4.Transfer 5.History 6.Inbox 7.Change PIN 8.Chatbot 9.Mirror 10.Explain Txn 11.Karma 12.Logout\nChoice: ");
        scanf("%d",&ch);
        switch(ch){
            case 1:
                printf("Balance: %.2f\n",u->balance);
                if(u->balance<MIN_BALANCE) printf("⚠️ Min balance not maintained.\n");
                break;
            case 2:
                printf("Amount: "); scanf("%f",&amt);
                if(amt>0){
                    u->balance+=amt; u->depositCount++;
                    saveUser(*u); logTxn(u->accNo,"DEPOSIT",amt,0);
                    saveKarma(u->accNo, loadKarma(u->accNo)+2);
                }
                break;
            case 3:
                printf("Amount: "); scanf("%f",&amt);
                if(amt<=0) printf("❌ Invalid amount.\n");
                else if(amt>u->balance) printf("❌ Insufficient funds.\n");
                else{
                    u->balance-=amt; u->withdrawCount++;
                    saveUser(*u); logTxn(u->accNo,"WITHDRAW",amt,0);
                    saveKarma(u->accNo, loadKarma(u->accNo)-1);
                    applyMinBalanceRule(u);
                }
                break;
            case 4: transferMoney(u); break;
            case 5: showHistory(u->accNo); break;
            case 6: showInbox(u->accNo); break;
            case 7: changePIN(u); break;
            case 8: smartChatbot(u); break;
            case 9: financialMirror(u->accNo); break;
            case 10: explainLastTransaction(u->accNo); break;
            case 11: showKarma(u->accNo); break;
            case 12:
                generateMonthlyEmail(u->accNo);
                saveUser(*u);
                printf("👋 Logout\n");
                break;
        }
    }while(ch!=12);
}
void createAccount(){
    User u={0};
    printf("Name: "); getchar(); fgets(u.name,50,stdin); u.name[strcspn(u.name,"\n")]=0;
    printf("Email: "); fgets(u.email,50,stdin); u.email[strcspn(u.email,"\n")]=0;
    printf("Phone: "); fgets(u.phone,15,stdin); u.phone[strcspn(u.phone,"\n")]=0;
    if(!verifyOTP("email")||!verifyOTP("phone")){ printf("❌ OTP failed.\n"); return; }
    char pinStr[10];
    printf("Set 4-digit PIN: "); inputHidden(pinStr,sizeof(pinStr));
    u.pin = atoi(pinStr);
    do{ u.accNo = rand()%90000+10000; }while(loadUser(u.accNo,&u));
    saveUser(u); addToIndex(u.accNo); saveKarma(u.accNo,0);
    pushInbox(u.accNo,"Welcome! Account created.");
    printf("✅ Account created! AccNo: %d\n",u.accNo);
}
void login(){
    int accNo, tries=3; User u; char pinStr[10];
    printf("Account No: "); scanf("%d",&accNo);
    if(isGlobalFreeze()){ printf("🧊 System under maintenance.\n"); return; }
    if(isBlocked(accNo)){ printf("❌ Frozen by admin.\n"); return; }
    if(!loadUser(accNo,&u)){ printf("❌ Not found.\n"); return; }
    while(tries--){
        printf("PIN: "); inputHidden(pinStr,sizeof(pinStr));
        if(atoi(pinStr)==u.pin){ bankMenu(&u); return; }
        printf("Wrong PIN. Tries left: %d\n",tries);
    }
}
void adminPanel(){
    char pinStr[10]; int ch,acc;
    printf("Admin PIN: "); inputHidden(pinStr,sizeof(pinStr));
    if(atoi(pinStr)!=ADMIN_PIN){ printf("❌ Wrong PIN\n"); return; }
    do{
        printf("\nADMIN: 1.List Users 2.Block 3.Unblock 4.Fraud Radar 5.Broadcast 6.Global Freeze ON 7.Global Freeze OFF 8.Exit\nChoice: ");
        scanf("%d",&ch);
        if(ch==1){
            FILE *fp=fopen(DATA_DIR "accounts_index.txt","r"); int a; User u;
            printf("\n--- USERS ---\n");
            if(fp){
                while(fscanf(fp,"%d",&a)!=EOF)
                    if(loadUser(a,&u))
                        printf("Acc:%d | %s | %.2f | %s\n",u.accNo,u.name,u.balance,isBlocked(u.accNo)?"BLOCKED":"ACTIVE");
                fclose(fp);
            }
        } else if(ch==2){ printf("AccNo: "); scanf("%d",&acc); blockAccount(acc); logAdminAction("Account blocked"); }
        else if(ch==3){ printf("AccNo: "); scanf("%d",&acc); unblockAccount(acc); logAdminAction("Account unblocked"); }
        else if(ch==4){ adminFraudRadar(); logAdminAction("Fraud radar viewed"); }
        else if(ch==5){ adminBroadcast(); }
        else if(ch==6){ toggleGlobalFreeze(1); }
        else if(ch==7){ toggleGlobalFreeze(0); }
    }while(ch!=8);
}

// ---------------- MAIN ----------------
int main(){
    srand(time(0));
    ensureDataDir();
    int ch;
    do{
        printf("\n=== DIGITAL BANK V3 (FINAL FIXED) ===\n");
        printf("1.Create Account\n2.Login\n3.Admin Panel\n4.Exit\nChoice: ");
        scanf("%d",&ch);
        if(ch==1) createAccount();
        else if(ch==2) login();
        else if(ch==3) adminPanel();
    }while(ch!=4);
    return 0;
}
