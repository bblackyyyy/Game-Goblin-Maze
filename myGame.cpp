#include <iostream>
#include <random>
#include <algorithm>
#include <array>
#include <vector>
#include <cmath>
#include <iomanip>
#include <thread>
#include <chrono>
//#include <ctime>
#include <time.h>
#ifdef WIN32
#include <conio.h>






int read_key()
{
    int c = _getch();
    if (c == 0 || c == 224)
        c = _getch() + (c<<8);
    return c;
}
#else
#include <termios.h>
#include <unistd.h>

// int timer(int countdown){
//   while (countdown > 0) {
//         std::cout << "Time remaining: " << countdown << " seconds" << std::endl;
//         std::this_thread::sleep_for(std::chrono::seconds(1));
//         countdown--;
//     }
// }








int read_key()
{
   char buf = 0;
   struct termios old = {0};
   fflush(stdout);
   if(tcgetattr(0, &old) < 0)
       perror("tcsetattr()");
   old.c_lflag &= (~ICANON) & (~ECHO);
   old.c_cc[VMIN] = 1;
   old.c_cc[VTIME] = 0;
   if(tcsetattr(0, TCSANOW, &old) < 0)
       perror("tcsetattr ICANON");
   if(read(0, &buf, 1) < 0)
       perror("read()");
   old.c_lflag |= ICANON | ECHO;
   if(tcsetattr(0, TCSADRAIN, &old) < 0)
       perror("tcsetattr ~ICANON");
   return buf;
}
#endif



using namespace std;




struct ivec2
{
    int x = 0;
    int y = 0;
    ivec2 operator*(int s) const { return { x * s,y * s }; }
    ivec2 operator+(ivec2 q) const { return { x + q.x,y + q.y }; }
    ivec2 operator%(ivec2 q) const { return { x % q.x,y % q.y }; }
    bool operator ==(ivec2 q) const {return x==q.x && y==q.y;}
};

enum {FLOOR =0, EXIT, TREASURE, ORB, ENEMY, HERO};
constexpr array<ivec2, 4> nbo{ivec2{1,0},{0,1},{-1,0},{0,-1}};
constexpr char symbols[] = " >$Yg@";
constexpr ivec2 dims{ 33,21 };



struct Player{
float playerHealth = 100.00; //hp
float playerAttack = 10.00;  // Attack
float playerProtection = 0.03;
};
int level = 1;


struct Goblin{
float enemy_health = 50;
float enemy_attack = 19.2 * level;
bool angry =0;
};


Player n;
Goblin g;



int keys[10]= {'d','w','a','s','g','r','1','2','3','4'};
array<uint8_t, dims.x* dims.y> map;
int treasure = 0;

int steps = 0;
int dynamite = 0;
ivec2 hero;
vector<pair<ivec2,int>> enemy_position_and_last_direction;

void bit_set(uint8_t& bits, int pos) { bits |= 1 << pos; }
void bit_clear(uint8_t& bits, int pos) { bits &= ~(1UL << pos); }
bool bit_test(uint8_t& bits, int pos) { return bits & (1 << pos); }


bool oob(ivec2 p) { return p.x < 0 || p.y < 0 || p.x >= dims.x || p.y >= dims.y;  }
bool oobb(ivec2 p) { return p.x <= 0 || p.y <= 0 || p.x >= (dims.x-1) || p.y >= (dims.y-1); }
uint8_t& mapelem(ivec2 p) { return map[p.x + p.y * dims.x]; }

void game_start_menu()
{
    int i;
    cout<<"Standart HP: 100; Damage: 10; Backpack space: 8; Protection: 0.03" << '\n';
    cout<<"Choose your race:"<<'\n'<< "1. Human:  +0 HP; +0 damage ;  +0.02 Protection "<<'\n' <<"2. Elf:  +5 HP;  +20 damage;  0 Protection "<< '\n'<<"3. Dwarf: -50HP; +35 damage; ;  +0.1 Protection  "<<'\n' <<"4. Goblin Slayer: +0 HP; +20 attack;  +0 Protection; "<<'\n';
    cin>>i;
    if(i == 1){
      n.playerProtection+=0.02;
    }
    if(i ==2){
    n.playerHealth+=5;
    n.playerAttack+=20;
    n.playerProtection=0;
    }
    if(i == 3){
      n.playerHealth-=50;
      n.playerAttack+=35;
      n.playerProtection+=0.1;
    }
    if(i == 4){
      n.playerAttack+=20;
    }



}


void create_iteration(ivec2 p)
{

    array<int, 4> dir4_index{ 0,1,2,3 };
    random_shuffle(dir4_index.begin(), dir4_index.end());
    for (auto i : dir4_index)
    {
        auto nb = p + nbo[i] * 2;
        if (oobb(nb) || mapelem(nb) != 0)
            continue;
        mapelem(nb) = 1;
        mapelem(p + nbo[i]) = 1;
        create_iteration(nb);
    }
}

void place_feature(int feature_bit, int num)
{
    int floors_traversed = 0;
    int place_at_multiples_of = 50 + (rand()%100);
    while(num > 0)
        for(auto& m : map)
            if(m == 1 && (++floors_traversed % place_at_multiples_of) == 0)
            {
                bit_set( m, feature_bit);
                --num;
            }
}
void clearscreen() {
  system("cls");
 }
//cout << "\033[2J\033[0;0H"; }







struct Item{
  string name;
  float effect;
  unsigned int amount;
  int id;
};


vector<Item> Inventory = {{"sword", 5, 0,0},{"armour", 0.03, 0,1},{"health_poison", 10, 0,2},{"strenght_poison", 5, 0,3} };

struct wall{
  char c = '#';
};

wall a;
void display()
{
    clearscreen();
    for (int y = dims.y-1 ; y >= 0; --y)
    {
        for (int x = 0; x < dims.x; ++x)
        {
            auto m = map[x+y*dims.x];
            char c = a.c;
            for (int j = 0; j <= HERO; ++j)
                if (m & (1 << j))
                    c = symbols[j];
            cout << c;
        }
        if(y == (dims.y - 1)){
        cout << "  Level:    " << level;}
        else if (y == (dims.y - 2)){
        cout << "  Coins: " << treasure;}
        else if (y == (dims.y - 3)){
        cout << "  Dynamite: " << dynamite;}
        else if (y == (dims.y - 4)){
        cout << "  Steps:    " << steps;}
        else if (y == (dims.y - 5)){
        cout <<setprecision(3)  << "  HP:     " << n.playerHealth;
        }
        else if (y == (dims.y - 6)){

         cout <<setprecision(3) << "  Attack:  " << n.playerAttack;}
         else if (y == (dims.y - 7)){

          cout <<setprecision(3) << "  armour:  " << n.playerProtection;}

          else if (y == (dims.y - 8)){

           cout <<setprecision(3) << "  Inventory:  " ;
         }
         else if (y == (dims.y - 9)){

          cout << "    1.Sword  "  <<  Inventory[0].amount << "/2   ^3 Coins";
        }
        else if (y == (dims.y - 10)){

         cout << "    2.Armor  "  <<  Inventory[1].amount << "/2  ^4 Coins";
       }
       else if (y == (dims.y - 11)){

        cout << "    3.Health_poison  "  <<  Inventory[2].amount << "/2  ^3 Coins";

      }
      else if (y == (dims.y - 12)){

       cout << "    4.Strenght poison "  <<  Inventory[3].amount << "/2  ^8 Coins ";
     }


        cout << '\n';
    }
}

void create_level(int seed)
{
    ++level;
    srand(seed);
//map generation
if(seed%5){
  map.fill(0);
  ivec2 p = { (rand() % (dims.x-1)) | 1, (rand() % (dims.y-1)) | 1 };
  mapelem(p) = 1;
  create_iteration(p);
  bit_set( mapelem(p) , HERO);
  place_feature(TREASURE, 4);
  place_feature(ENEMY, 6+level);
  place_feature(EXIT, 1);
  int orbchance = 10*(level-15);
  if( (rand()%100) < orbchance)
    place_feature(ORB, 1);
  enemy_position_and_last_direction.clear();
  hero = p;
  for (p.y = 0; p.y < dims.y; ++p.y)
      for (p.x = 0; p.x < dims.x; ++p.x)
          if (bit_test(mapelem(p), ENEMY))
              enemy_position_and_last_direction.emplace_back(p, -1 /*invalid direction*/);

  // remove some walls
  int to_remove = 5;
  while(to_remove > 0)
  {
      auto& m = mapelem(ivec2{rand(),rand()}%dims);
      if (m == 0)
      {
          m = 1;
          --to_remove;
      }
  }
}
else{ //ruined level
map.fill(0);
ivec2 p = { (rand() % (dims.x-1)) | 1, (rand() % (dims.y-1)) | 1 };
mapelem(p) = 1;
create_iteration(p);
bit_set( mapelem(p) , HERO);
place_feature(TREASURE, 8);
place_feature(ENEMY, 10+level);
place_feature(EXIT, 1);
int orbchance = 10*(level-15);
if( (rand()%100) < orbchance)
  place_feature(ORB, 1);
enemy_position_and_last_direction.clear();
hero = p;
for (p.y = 0; p.y < dims.y; ++p.y)
    for (p.x = 0; p.x < dims.x; ++p.x)
        if (bit_test(mapelem(p), ENEMY))
            enemy_position_and_last_direction.emplace_back(p, -1 /*invalid direction*/);

// remove some walls
int to_remove = 200 ;
while(to_remove > 0)
{
    auto& m = mapelem(ivec2{rand(),rand()}%dims);
    if (m == 0)
    {
        m = 1;
        --to_remove;
    }
}
}



}

void startgame()
{
    treasure = 0;
    level = 0;
    steps = 0;
    dynamite = 3;
    create_level(time(NULL));
}


void use_dynamite()
{
  --dynamite;
  auto& vec = enemy_position_and_last_direction;
  for(int i=0;i<4;++i)
  {
    auto p = (hero + nbo[i] + dims)%dims;
    auto& m = mapelem(p);
    if(bit_test(m, ENEMY))
      vec.erase(std::remove_if(vec.begin(), vec.end(), [p](auto x){return x.first == p;}), vec.end());
    bit_clear(m, ENEMY);
    bit_set(m, FLOOR);
  }
}

void win()
{
 clearscreen();
  cout<<"\n\n"<< R"(
 __       __  __
|  \  _  |  \|  \
| $$ / \ | $$ \$$ _______
| $$/  $\| $$|  \|       \
| $$  $$$\ $$| $$| $$$$$$$\
| $$ $$\$$\$$| $$| $$  | $$
| $$$$  \$$$$| $$| $$  | $$
| $$$    \$$$| $$| $$  | $$
 \$$      \$$ \$$ \$$   \$$
)";
cout << "\n... as the mighty orb is yours to wield!\n\n";
constexpr char tabs[] = "\t\t\t\t\t";
cout << tabs<<"Coins    : "<<treasure<<'\n';
cout << tabs<<"Steps       : "<<steps<<"\n\n";
cout << tabs<<"Dynamite    : "<<dynamite<<"\n\n";
cout <<setprecision(2) << tabs<<"HP      : "<<floor(n.playerHealth)<<"\n\n";
cout <<setprecision(2) << tabs<<"Attack     : "<<n.playerAttack<<"\n\n";
cout << tabs<<"Total score : "<< int((treasure + 10*dynamite)*1000/(float)(steps+1))<<'\n';
//cout << tabs<<"HP    : "<<playerHealth<<"\n\n";
getchar();
startgame();
}


void lost()
{
  clearscreen();
  cout<<"\n\n"<< R"(
 __        ______    ______  ________
|  \      /      \  /      \|        \
| $$     |  $$$$$$\|  $$$$$$\\$$$$$$$$
| $$     | $$  | $$| $$___\$$  | $$
| $$     | $$  | $$ \$$    \   | $$
| $$     | $$  | $$ _\$$$$$$\  | $$
| $$_____| $$__/ $$|  \__| $$  | $$
| $$     \\$$    $$ \$$    $$  | $$
 \$$$$$$$$ \$$$$$$   \$$$$$$    \$$
)";
cout << "\t\t\t\t\t \n...\n\n";
constexpr char tabs[] = "\t\t\t\t\t";
cout << tabs<<"Coins    : "<<treasure<<"\n";
cout << tabs<<"Steps       : "<<steps<<"\n";
cout << tabs<<"Dynamite    : "<<dynamite<<"\n";
cout << setprecision(4) << tabs<<"HP    : "<<0<<"\n";
cout << setprecision(4) << tabs<<"Attack    : "<<n.playerAttack<<"\n";
cout <<tabs<<"Total score : "<< int((treasure + 10*dynamite)*1000/(float)(steps+1))<<'\n';
//cout << tabs<<"HP    : "<<playerHealth<<"\n\n";
getchar();
startgame();
}

void welcome()
{
  cout << "\n\n"<<R"(
  $$$$$$\            $$\       $$\ $$\                 $$\      $$\
$$  __$$\           $$ |      $$ |\__|                $$$\    $$$ |
$$ /  \__| $$$$$$\  $$$$$$$\  $$ |$$\ $$$$$$$\        $$$$\  $$$$ | $$$$$$\  $$$$$$$$\  $$$$$$\
$$ |$$$$\ $$  __$$\ $$  __$$\ $$ |$$ |$$  __$$\       $$\$$\$$ $$ | \____$$\ \____$$  |$$  __$$\
$$ |\_$$ |$$ /  $$ |$$ |  $$ |$$ |$$ |$$ |  $$ |      $$ \$$$  $$ | $$$$$$$ |  $$$$ _/ $$$$$$$$ |
$$ |  $$ |$$ |  $$ |$$ |  $$ |$$ |$$ |$$ |  $$ |      $$ |\$  /$$ |$$  __$$ | $$  _/   $$   ____|
\$$$$$$  |\$$$$$$  |$$$$$$$  |$$ |$$ |$$ |  $$ |      $$ | \_/ $$ |\$$$$$$$ |$$$$$$$$\ \$$$$$$$\
 \______/  \______/ \_______/ \__|\__|\__|  \__|      \__|     \__| \_______|\________| \_______|
)" <<"\n\t\t\t\tWelcome to my game!\n\n";
}

// void move(int feature_bit, ivec2& from, ivec2 to)
// {
//     bit_clear( mapelem(from), feature_bit); // clear
//     bit_set( mapelem(to), feature_bit); // set
//     from = to;
//     auto v = mapelem(to);
//     if (bit_test(v, HERO) && bit_test(v, ENEMY))
//         startgame();
//     else if (bit_test(v, HERO) && bit_test(v, TREASURE))
//     {
//         ++treasure;
//         bit_clear(mapelem(to), TREASURE);
//     }
//     else if (bit_test(v, HERO) && bit_test(v, ORB))
//         win();
//     else if (bit_test(v, HERO) && bit_test(v, EXIT))
//         create_level(time(NULL));
// }

//bool in_battle = 0;

void move(int feature_bit, ivec2& from, ivec2 to)
{
    bit_clear(mapelem(from), feature_bit);
    bit_set(mapelem(to), feature_bit);
    from = to;
    auto v = mapelem(to);

    if (bit_test(v, HERO) && bit_test(v, ENEMY))
    {

        int enemyIndex = -1;
        for (size_t i = 0; i < enemy_position_and_last_direction.size(); ++i)
        {
            if (enemy_position_and_last_direction[i].first == to)
            {
                enemyIndex = static_cast<int>(i);
                break;
            }
        }

        if (enemyIndex != -1)
        {
            //in_battle =1;
            g.angry =1;
            g.enemy_health*level+25;
            enemy_position_and_last_direction[enemyIndex].second -= n.playerAttack;
            g.enemy_health-=n.playerAttack;
            float damage =
            n.playerHealth =  n.playerHealth - ((g.enemy_attack*n.playerProtection)*g.enemy_attack  )   ;

            if(g.enemy_health == 0){
              //in_battle =0;
              g.angry = 0;
              int rand_money  = rand()%(2+level);
              treasure+=rand_money;
              int chance = 10*(level-2);
              if( (rand()%100) < chance){
                int index = rand()%4;
                if(Inventory[index].amount > 2){
                  index = rand()%2;
                    Inventory[index].amount++;
                }
              }



            if (enemy_position_and_last_direction[enemyIndex].second <= 0)
            {

                enemy_position_and_last_direction.erase(enemy_position_and_last_direction.begin() + enemyIndex);
                bit_clear(mapelem(to), ENEMY);
                bit_set(mapelem(to), FLOOR);
            }}
        }
    }
    // else if (bit_test(v, HERO) && bit_test(v, ENEMY))
    //     lost(); // Player vs Enemy combat (you can customize this logic)
    else if (bit_test(v, HERO) && bit_test(v, TREASURE))
    {

        treasure+=rand()%10;
        int index = rand()%4;
        if(Inventory[index].amount > 2){
          index = rand()%4;
        }
        Inventory[index].amount++;

        bit_clear(mapelem(to), TREASURE);
    }
    else if (bit_test(v, HERO) && bit_test(v, ORB))
        win();
    else if (bit_test(v, HERO) && bit_test(v, EXIT))
        create_level(time(NULL));
    else if (bit_test(v, HERO) && bit_test(v, FLOOR))
    {

        if (n.playerHealth < 100.0)
        {
            n.playerHealth += 0.01;
            if (n.playerHealth > 100)
                n.playerHealth = 100;
        }
    }
}








int main() {

    welcome();
    game_start_menu();
    cout<<"\n\n";

    startgame();
    cout<<"\n\n";
    while (true)
    {

        bool used = 0;
        display();
        int c = read_key();
        if(c==keys[6]){
          //using sword ++ attack
             if(Inventory[0].amount > 0){
               n.playerAttack+=Inventory[0].effect;
               Inventory[0].amount-=1;
               used = 1;

             }

             if(Inventory[0].amount == 0){
               if(treasure >= 3){
                 treasure-=3;
                 Inventory[0].amount++;
               }
             }
        }
        if(c==keys[7]){
          //using armor ++ protection
          if(Inventory[1].amount > 0){
            n.playerProtection+=Inventory[1].effect;
            Inventory[1].amount-=1;
          }

          if(Inventory[1].amount == 0){
            if(treasure >= 4){
              treasure-=4;
              Inventory[1].amount++;
            }
          }

        }
        if(c==keys[8]){
          //using health_poison ++ health
          if(Inventory[2].amount > 0){
            n.playerHealth+=Inventory[2].effect;
            Inventory[2].amount-=1;
          }

          if(Inventory[2].amount == 0){
            if(treasure >= 3){
              treasure-=3;
              Inventory[2].amount++;
            }
          }

        }
        if(c==keys[9]){
          //using strenght_poison ++ attack
          if(Inventory[3].amount > 0){
            n.playerAttack+=Inventory[3].effect;
            Inventory[3].amount-=1;
          }

          if(Inventory[3].amount == 0){
            if(treasure >= 8){
              treasure-=8;
              Inventory[3].amount++;
            }
          }

        }

        if (c == keys[5])
          startgame();
        if(c == keys[4] && dynamite > 0)
          use_dynamite();
        else
        {
          ivec2 dir;
          for(int i=0;i<4;++i)
            if (c == keys[i])
                dir = nbo[i];
          auto q = (hero + dir + dims)%dims;
          if (mapelem(q) > 0)
          {
              move(HERO, hero, q);
              ++steps;
          }
        }

        if(!g.angry){//!in_battle){
        for (auto& e : enemy_position_and_last_direction)
        {
            int i0 = rand() % 4; // pick random start dir
            bool moved = false;
            for (int i = 0; i<4;++i)
            {
                int iDirOpposite = (i0 + i+2) % 4;
                if (iDirOpposite == e.second)
                    continue;
                int iDir = (i0 + i) % 4;
                auto p = (e.first + nbo[iDir] + dims)%dims;
                if (bit_test(mapelem(p), FLOOR))
                {
                    move(ENEMY, e.first, p);
                    e.second = iDir;
                    moved = true;
                    break;
                }
            }
            if (!moved) // failed  move
            {
                int iDirOpposite = (e.second + 2) % 4;
                move(ENEMY, e.first, e.first + nbo[iDirOpposite]);
                e.second = iDirOpposite;
            }

        }

      }

        if(n.playerHealth < 0 ){
            //playerHealth =0;
            lost();
        }


    }
}

