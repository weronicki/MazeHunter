#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define SDA_PIN 4
#define SCL_PIN 7
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Buttons
#define BTN_UP 0
#define BTN_DOWN 1
#define BTN_LEFT 2
#define BTN_RIGHT 3
#define BTN_A 20
#define BTN_B 19
#define BTN_MENU1 18
#define BTN_MENU2 14

#define MAP_WIDTH 16
#define MAP_HEIGHT 16

// Starting wall that is directly in front of the player after reset.
#define RUN_WALL_X (MAP_WIDTH - 4)
#define RUN_WALL_Y (MAP_HEIGHT - 2)

// 16x16 black wall texture containing only the text "RUN !".
// A set bit means that the corresponding wall pixel is black.
const uint16_t runWallText[16] PROGMEM = {
  0x0000,
  0x0000,
  0x0000,
  0x0000,
  0x0000,
  0x6554, //  R:110  U:101  N:101  !:1
  0x5574, //  R:101  U:101  N:111  !:1
  0x6574, //  R:110  U:101  N:111  !:1
  0x5570, //  R:101  U:101  N:111  !:0
  0x5754, //  R:101  U:111  N:101  !:1
  0x0000,
  0x0000,
  0x0000,
  0x0000,
  0x0000,
  0x0000
};

bool isRunTextPixel(int texX, int texY) {
  if (texX < 0 || texX > 15 || texY < 0 || texY > 15) return false;
  uint16_t row = pgm_read_word(&runWallText[texY]);
  return row & (0x8000u >> texX);
}

// Enemy sprite mask and pixel data (a spooky 8x8 ghost scaled to 16x16)
const uint8_t enemyMask[8] PROGMEM = {
  0x3C, 0x7E, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xDB
};
const uint8_t enemySprite[8] PROGMEM = {
  0x3C, 0x7E, 0xDB, 0xFF, 0xFF, 0xC3, 0x81, 0xDB
};

// Exit Door Texture (checkerboard)
const uint8_t doorTexture[8] PROGMEM = {
  0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55
};

// 1: wall, 2: exit door, 0: floor - Maze Layout
uint8_t worldMap[MAP_WIDTH][MAP_HEIGHT];

float posX = 1.5, posY = 13.5;  // start position
float dirX = 1.0, dirY = 0.0;
float planeX = 0.0, planeY = 0.66;

struct Sprite {
  float x;
  float y;
};

#define numSprites 1
Sprite sprites[numSprites] = {
  {1.5, 1.5} // Will be randomized in setup
};

bool isGameOver = false;
bool isGameWon = false;

// Enemy AI state is global so it can be reset with the game.
int enemyStateTimer = 0;
int enemyTargetX = -1;
int enemyTargetY = -1;
bool isWandering = false;

// ZBuffer used to handle sprite overlap with walls!
float ZBuffer[SCREEN_WIDTH];
int spriteOrder[numSprites];
float spriteDistance[numSprites];

void spawnEnemy() {
  bool spawned = false;
  while (!spawned) {
    int rx = random(1, MAP_WIDTH - 1);
    int ry = random(1, MAP_HEIGHT - 1);
    // don't spawn in walls, and don't spawn right next to the player
    if (worldMap[rx][ry] == 0 && (abs(rx - posX) > 3 || abs(ry - posY) > 3)) {
      sprites[0].x = rx + 0.5f;
      sprites[0].y = ry + 0.5f;
      spawned = true;
    }
  }
}

void generateMaze() {
  // 1. Fill completely with walls
  for (int x = 0; x < MAP_WIDTH; x++) {
    for (int y = 0; y < MAP_HEIGHT; y++) {
      worldMap[x][y] = 1; 
    }
  }

  // 2. Iterative DFS Maze Generation
  // Using simple arrays as a stack
  uint8_t stackX[128];
  uint8_t stackY[128];
  int stackSize = 0;

  // Start carving from 1,1
  int cx = 1;
  int cy = 1;
  worldMap[cx][cy] = 0;
  stackX[stackSize] = cx;
  stackY[stackSize] = cy;
  stackSize++;

  while (stackSize > 0) {
    // Look for unvisited neighbors (distance 2 away)
    int unvisitedX[4];
    int unvisitedY[4];
    int numUnvisited = 0;

    int dx[4] = {0, 0, -2, 2};
    int dy[4] = {-2, 2, 0, 0};

    for (int i = 0; i < 4; i++) {
        int nx = cx + dx[i];
        int ny = cy + dy[i];
        if (nx > 0 && nx < MAP_WIDTH - 1 && ny > 0 && ny < MAP_HEIGHT - 1) {
            if (worldMap[nx][ny] == 1) {
                unvisitedX[numUnvisited] = nx;
                unvisitedY[numUnvisited] = ny;
                numUnvisited++;
            }
        }
    }

    if (numUnvisited > 0) {
        // Pick random neighbor
        int r = random(0, numUnvisited);
        int nx = unvisitedX[r];
        int ny = unvisitedY[r];

        // Carve path
        worldMap[nx][ny] = 0;
        // Carve wall between current and neighbor
        worldMap[cx + (nx - cx) / 2][cy + (ny - cy) / 2] = 0;

        // Push current to stack, make neighbor new current
        stackX[stackSize] = cx;
        stackY[stackSize] = cy;
        stackSize++;
        
        cx = nx;
        cy = ny;
    } else {
        // Backtrack
        stackSize--;
        cx = stackX[stackSize];
        cy = stackY[stackSize];
    }
  }

  // Set Exit Door at top left
  worldMap[1][1] = 2;
  
  // Ensure start area is clear
  worldMap[MAP_WIDTH-2][MAP_HEIGHT-2] = 0;
  worldMap[MAP_WIDTH-3][MAP_HEIGHT-2] = 0;
}

void resetGame() {
  generateMaze();
  
  posX = MAP_WIDTH - 1.5; // Spawn at bottom right
  posY = MAP_HEIGHT - 1.5;
  dirX = -1.0; dirY = 0.0;
  planeX = 0.0; planeY = -0.66;
  isGameOver = false;
  isGameWon = false;

  enemyStateTimer = 0;
  enemyTargetX = -1;
  enemyTargetY = -1;
  isWandering = false;

  spawnEnemy();
}

void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.clearDisplay();
  
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(BTN_MENU1, INPUT_PULLUP);
  pinMode(BTN_MENU2, INPUT_PULLUP);

  randomSeed(analogRead(0));
  resetGame();
}

void loop() {
  if (isGameOver) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(38, 28);
    display.print("GAME OVER");
    display.display();
    
    delay(3000); // Wait 3 seconds
    resetGame();
    return;
  }

  if (isGameWon) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(38, 28);
    display.print("YOU ESCAPED!");
    display.display();
    
    delay(3000); // Wait 3 seconds
    resetGame();
    return;
  }

  display.clearDisplay();

  // Draw walls and fill ZBuffer
  for (int x = 0; x < SCREEN_WIDTH; x++) {
    float cameraX = 2 * x / (float)SCREEN_WIDTH - 1; 
    float rayDirX = dirX + planeX * cameraX;
    float rayDirY = dirY + planeY * cameraX;

    int mapX = int(posX);
    int mapY = int(posY);

    float sideDistX;
    float sideDistY;

    float deltaDistX = (rayDirX == 0) ? 1e30 : abs(1 / rayDirX);
    float deltaDistY = (rayDirY == 0) ? 1e30 : abs(1 / rayDirY);
    float perpWallDist;

    int stepX;
    int stepY;
    int hit = 0;
    int side;

    if (rayDirX < 0) {
      stepX = -1;
      sideDistX = (posX - mapX) * deltaDistX;
    } else {
      stepX = 1;
      sideDistX = (mapX + 1.0 - posX) * deltaDistX;
    }
    if (rayDirY < 0) {
      stepY = -1;
      sideDistY = (posY - mapY) * deltaDistY;
    } else {
      stepY = 1;
      sideDistY = (mapY + 1.0 - posY) * deltaDistY;
    }

    while (hit == 0) {
      if (sideDistX < sideDistY) {
        sideDistX += deltaDistX;
        mapX += stepX;
        side = 0;
      } else {
        sideDistY += deltaDistY;
        mapY += stepY;
        side = 1;
      }
      if (worldMap[mapX][mapY] > 0) hit = 1;
    }

    if (side == 0) perpWallDist = (sideDistX - deltaDistX);
    else           perpWallDist = (sideDistY - deltaDistY);
    if(perpWallDist < 0.1) perpWallDist = 0.1;

    // Save distance for sprite casting
    ZBuffer[x] = perpWallDist; 

    int lineHeight = (int)(SCREEN_HEIGHT / perpWallDist);

    int drawStart = -lineHeight / 2 + SCREEN_HEIGHT / 2;
    if (drawStart < 0) drawStart = 0;
    int drawEnd = lineHeight / 2 + SCREEN_HEIGHT / 2;
    if (drawEnd >= SCREEN_HEIGHT) drawEnd = SCREEN_HEIGHT - 1;

    int blockType = worldMap[mapX][mapY];

    float wallX;
    if (side == 0) wallX = posY + perpWallDist * rayDirY;
    else           wallX = posX + perpWallDist * rayDirX;
    wallX -= floor((wallX));

    int texX = int(wallX * 16.0);
    if(side == 0 && rayDirX > 0) texX = 16 - texX - 1;
    if(side == 1 && rayDirY < 0) texX = 16 - texX - 1;

    float step_tex = 16.0 / lineHeight;
    float texPos = (drawStart - SCREEN_HEIGHT / 2 + lineHeight / 2) * step_tex;

    for (int y = drawStart; y < drawEnd; y++) {
      int texY = (int)texPos & 15;
      texPos += step_tex;
      
      if (blockType == 2) {
        // Exit Door Texture!
        int tx = texX / 2;
        int ty = texY / 2;
        uint8_t rowData = pgm_read_byte(&doorTexture[ty]);
        if (rowData & (1 << (7 - tx))) {
            display.drawPixel(x, y, SSD1306_WHITE);
        } else {
            // Draw nothing for the dark square to look checkerboard
        }
      } else {
        // The wall directly in front of the starting position has a
        // perspective-correct black "RUN !" texture painted on it.
        bool runWall = (mapX == RUN_WALL_X && mapY == RUN_WALL_Y);

        if (runWall && isRunTextPixel(15 - texX, texY)) {
          display.drawPixel(x, y, SSD1306_BLACK);
        } else if (side == 1) {
          if ((y % 2) == 0) display.drawPixel(x, y, SSD1306_WHITE);
        } else {
          display.drawPixel(x, y, SSD1306_WHITE);
        }
      }
    }
  }

  // Draw Sprites
  // Sort sprites by distance from player to draw furthest to nearest
  for(int i = 0; i < numSprites; i++) {
    spriteOrder[i] = i;
    spriteDistance[i] = ((posX - sprites[i].x) * (posX - sprites[i].x) + (posY - sprites[i].y) * (posY - sprites[i].y));
  }
  for(int i = 0; i < numSprites - 1; i++) {
    for(int j = 0; j < numSprites - i - 1; j++) {
      if(spriteDistance[spriteOrder[j]] < spriteDistance[spriteOrder[j+1]]) {
         int temp = spriteOrder[j];
         spriteOrder[j] = spriteOrder[j+1];
         spriteOrder[j+1] = temp;
      }
    }
  }

  // Project sprites onto camera plane
  for(int i = 0; i < numSprites; i++) {
    int spriteIndex = spriteOrder[i];
    float spriteX = sprites[spriteIndex].x - posX;
    float spriteY = sprites[spriteIndex].y - posY;

    float invDet = 1.0 / (planeX * dirY - dirX * planeY);
    float transformX = invDet * (dirY * spriteX - dirX * spriteY);
    float transformY = invDet * (-planeY * spriteX + planeX * spriteY); 

    // Do not project sprites behind the camera or exactly on the camera plane.
    if (transformY <= 0.01f) continue;

    int spriteScreenX = int((SCREEN_WIDTH / 2) * (1 + transformX / transformY));

    int spriteHeight = abs(int(SCREEN_HEIGHT / transformY)); 
    int drawStartY = -spriteHeight / 2 + SCREEN_HEIGHT / 2;
    if (drawStartY < 0) drawStartY = 0;
    int drawEndY = spriteHeight / 2 + SCREEN_HEIGHT / 2;
    if (drawEndY >= SCREEN_HEIGHT) drawEndY = SCREEN_HEIGHT - 1;

    int spriteWidth = abs(int(SCREEN_HEIGHT / transformY));
    int drawStartX = -spriteWidth / 2 + spriteScreenX;
    if (drawStartX < 0) drawStartX = 0;
    int drawEndX = spriteWidth / 2 + spriteScreenX;
    if (drawEndX >= SCREEN_WIDTH) drawEndX = SCREEN_WIDTH - 1;

    for (int stripe = drawStartX; stripe < drawEndX; stripe++) {
      int texX = int(256 * (stripe - (-spriteWidth / 2 + spriteScreenX)) * 16 / spriteWidth) / 256;
      
      // Prevent drawing if sprite is behind camera, off-screen, or hidden by wall ZBuffer
      if (transformY > 0 && stripe >= 0 && stripe < SCREEN_WIDTH && transformY < ZBuffer[stripe]) {
        for (int y = drawStartY; y < drawEndY; y++) {
          int d = y * 256 - SCREEN_HEIGHT * 128 + spriteHeight * 128;
          int texY = ((d * 16) / spriteHeight) / 256;
          
          if (texX >= 0 && texX < 16 && texY >= 0 && texY < 16) {
            int tx = texX / 2;
            int ty = texY / 2;
            uint8_t mask = pgm_read_byte(&enemyMask[ty]);
            // If the pixel is part of the ghost mask
            if (mask & (1 << (7 - tx))) {
              uint8_t spr = pgm_read_byte(&enemySprite[ty]);
              // Draw white if the sprite texture indicates white, else draw black (to obscure wall behind)
              if (spr & (1 << (7 - tx))) {
                display.drawPixel(stripe, y, SSD1306_WHITE);
              } else {
                display.drawPixel(stripe, y, SSD1306_BLACK);
              }
            }
          }
        }
      }
    }
  }

  // Draw Crosshair
  display.drawPixel(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, SSD1306_WHITE);
  display.drawPixel(SCREEN_WIDTH / 2 - 1, SCREEN_HEIGHT / 2, SSD1306_BLACK);
  display.drawPixel(SCREEN_WIDTH / 2 + 1, SCREEN_HEIGHT / 2, SSD1306_BLACK);
  display.drawPixel(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 1, SSD1306_BLACK);
  display.drawPixel(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 1, SSD1306_BLACK);

  // --- Minimap Toggle Logic ---
  static bool lastMenu2 = true;
  static bool showMinimap = false;
  bool currentMenu2 = digitalRead(BTN_MENU2);
  if (!currentMenu2 && lastMenu2) {
    showMinimap = !showMinimap;
  }
  lastMenu2 = currentMenu2;

  // --- Draw Minimap ---
  if (showMinimap) {
    // Top-left corner, 32x32 size (2 pixels per block)
    display.fillRect(0, 0, 34, 34, SSD1306_BLACK); // Background + 2px border
    display.drawRect(0, 0, 34, 34, SSD1306_WHITE); // White Border
    
    for (int y = 0; y < MAP_HEIGHT; y++) {
      for (int x = 0; x < MAP_WIDTH; x++) {
        uint8_t tile = worldMap[x][y];
        if (tile == 1) { // Wall
          display.fillRect(x * 2 + 1, y * 2 + 1, 2, 2, SSD1306_WHITE);
        } else if (tile == 2) { // Exit door
          display.drawPixel(x * 2 + 1, y * 2 + 1, SSD1306_WHITE);
          display.drawPixel(x * 2 + 2, y * 2 + 2, SSD1306_WHITE);
        }
      }
    }
    // Draw Player (Flashing or Just distinct)
    display.drawPixel(int(posX) * 2 + 1, int(posY) * 2 + 1, SSD1306_WHITE);
    // Player direction dot
    display.drawPixel(int(posX) * 2 + 1 + (dirX*2), int(posY) * 2 + 1 + (dirY*2), SSD1306_WHITE);
    
    // Draw Enemy (Flashing)
    if ((millis() / 200) % 2 == 0) {
      display.drawPixel(int(sprites[0].x) * 2 + 1, int(sprites[0].y) * 2 + 1, SSD1306_WHITE);
    }
  }

  display.display();

  bool btnLeft = !digitalRead(BTN_LEFT);
  bool btnRight = !digitalRead(BTN_RIGHT);
  bool btnA = !digitalRead(BTN_A);
  bool btnB = !digitalRead(BTN_B);

  float moveSpeed = 0.20; 
  float rotSpeed = 0.15;  

  // --- Hunter Enemy AI Pathfinding (BFS Flow Field) ---
  float enemySpeed = 0.015; // heavily reduced speed
  
  // --- Check Line of Sight (LoS) ---
  bool seesPlayer = false;
  float p_dx = posX - sprites[0].x;
  float p_dy = posY - sprites[0].y;
  float p_dist = sqrt(p_dx*p_dx + p_dy*p_dy);
  
  if (p_dist > 0.1) {
    float rayDirXLoS = p_dx / p_dist;
    float rayDirYLoS = p_dy / p_dist;
    
    int mapXLoS = int(sprites[0].x);
    int mapYLoS = int(sprites[0].y);
    int stepXLoS, stepYLoS;
    float sideDistXLoS, sideDistYLoS;
    float deltaDistXLoS = (rayDirXLoS == 0) ? 1e30 : abs(1 / rayDirXLoS);
    float deltaDistYLoS = (rayDirYLoS == 0) ? 1e30 : abs(1 / rayDirYLoS);
    
    if (rayDirXLoS < 0) { stepXLoS = -1; sideDistXLoS = (sprites[0].x - mapXLoS) * deltaDistXLoS; } 
    else                { stepXLoS = 1;  sideDistXLoS = (mapXLoS + 1.0 - sprites[0].x) * deltaDistXLoS; }
    if (rayDirYLoS < 0) { stepYLoS = -1; sideDistYLoS = (sprites[0].y - mapYLoS) * deltaDistYLoS; } 
    else                { stepYLoS = 1;  sideDistYLoS = (mapYLoS + 1.0 - sprites[0].y) * deltaDistYLoS; }
    
    bool hitWall = false;
    float distTraveled = 0;
    while (!hitWall && distTraveled < p_dist) {
      if (sideDistXLoS < sideDistYLoS) { sideDistXLoS += deltaDistXLoS; mapXLoS += stepXLoS; distTraveled = sideDistXLoS; } 
      else                             { sideDistYLoS += deltaDistYLoS; mapYLoS += stepYLoS; distTraveled = sideDistYLoS; }
      if (worldMap[mapXLoS][mapYLoS] > 0) hitWall = true;
    }
    seesPlayer = !hitWall;
  }
  
  // If we see the player, instantly snap out of wandering and hunt!
  if (seesPlayer) {
      isWandering = false;
      enemyStateTimer = 100; // Reset hunt timer
  }

  if (enemyStateTimer <= 0) {
    if (random(0, 10) < 4) { // 40% chance to start wandering
      isWandering = true;
      enemyStateTimer = random(50, 150); // frames to wander
      // Pick random open spot
      bool spotted = false;
      while(!spotted) {
        int rx = random(1, MAP_WIDTH-1);
        int ry = random(1, MAP_HEIGHT-1);
        if(worldMap[rx][ry] == 0) {
          enemyTargetX = rx;
          enemyTargetY = ry;
          spotted = true;
        }
      }
    } else {
      isWandering = false;
      enemyStateTimer = random(100, 300); // frames to hunt
    }
  }
  enemyStateTimer--;

  int px = isWandering ? enemyTargetX : int(posX);
  int py = isWandering ? enemyTargetY : int(posY);
  
  static uint8_t distMap[MAP_WIDTH][MAP_HEIGHT];
  for(int x=0; x<MAP_WIDTH; x++) {
    for(int y=0; y<MAP_HEIGHT; y++) {
      distMap[x][y] = 255;
    }
  }
  
  if(px >= 0 && px < MAP_WIDTH && py >= 0 && py < MAP_HEIGHT && worldMap[px][py] == 0) {
    distMap[px][py] = 0;
  } else {
    // Failsafe target player if wander spot is invalid
    distMap[int(posX)][int(posY)] = 0;
    px = int(posX); py = int(posY);
  }
  
  static uint8_t qx[256];
  static uint8_t qy[256];
  int qHead = 0, qTail = 0;
  qx[qTail] = px; 
  qy[qTail++] = py;
  
  while(qHead < qTail) {
    int cx = qx[qHead]; 
    int cy = qy[qHead++];
    int d = distMap[cx][cy];
    
    int dxOff[4] = {0, 0, -1, 1};
    int dyOff[4] = {-1, 1, 0, 0};
    
    for(int i=0; i<4; i++) {
        int nx = cx + dxOff[i];
        int ny = cy + dyOff[i];
        if(nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT) {
            if(worldMap[nx][ny] == 0 && distMap[nx][ny] == 255) {
                distMap[nx][ny] = d + 1;
                qx[qTail] = nx;
                qy[qTail++] = ny;
            }
        }
    }
  }
  
  int ex = int(sprites[0].x);
  int ey = int(sprites[0].y);
  
  int targetX = ex;
  int targetY = ey;
  uint8_t minDist = distMap[ex][ey];
  
  if (ex > 0 && distMap[ex-1][ey] < minDist) { minDist = distMap[ex-1][ey]; targetX = ex-1; targetY = ey; }
  if (ex < MAP_WIDTH-1 && distMap[ex+1][ey] < minDist) { minDist = distMap[ex+1][ey]; targetX = ex+1; targetY = ey; }
  if (ey > 0 && distMap[ex][ey-1] < minDist) { minDist = distMap[ex][ey-1]; targetX = ex; targetY = ey-1; }
  if (ey < MAP_HEIGHT-1 && distMap[ex][ey+1] < minDist) { minDist = distMap[ex][ey+1]; targetX = ex; targetY = ey+1; }
  
  float tpX = targetX + 0.5f;
  float tpY = targetY + 0.5f;
  if(minDist == 0 || minDist == 255) { 
    // Reached wander target, or no path found. End wandering early or try to just step to target.
    if (isWandering) {
        enemyStateTimer = 0; // stop wandering
    } else {
        tpX = posX; // Use actual float coordinates to touch the player
        tpY = posY;
    }
  }
  
  float edx = tpX - sprites[0].x;
  float edy = tpY - sprites[0].y;
  float edist = sqrt(edx*edx + edy*edy);
  
  if (edist > enemySpeed) {
    sprites[0].x += (edx/edist) * enemySpeed;
    sprites[0].y += (edy/edist) * enemySpeed;
  } else {
    sprites[0].x = tpX;
    sprites[0].y = tpY;
  }
  
  p_dx = posX - sprites[0].x;
  p_dy = posY - sprites[0].y;
  p_dist = sqrt(p_dx*p_dx + p_dy*p_dy);
  
  if (p_dist < 0.5) {
    isGameOver = true;
  }

  // --- Player Input Handling ---
  if (!digitalRead(BTN_MENU1)) {
    resetGame();
    delay(200);
  }

  if (btnA) {
    bool win = false;
    if (worldMap[int(posX + dirX * moveSpeed)][int(posY)] == 0) posX += dirX * moveSpeed;
    else if (worldMap[int(posX + dirX * moveSpeed)][int(posY)] == 2) win = true;
    
    if (worldMap[int(posX)][int(posY + dirY * moveSpeed)] == 0) posY += dirY * moveSpeed;
    else if (worldMap[int(posX)][int(posY + dirY * moveSpeed)] == 2) win = true;

    if (win) isGameWon = true;
  }
  if (btnB) {
    if (worldMap[int(posX - dirX * moveSpeed)][int(posY)] == 0) posX -= dirX * moveSpeed;
    if (worldMap[int(posX)][int(posY - dirY * moveSpeed)] == 0) posY -= dirY * moveSpeed;
  }
  if (btnLeft) {
    float oldDirX = dirX;
    dirX = dirX * cos(-rotSpeed) - dirY * sin(-rotSpeed);
    dirY = oldDirX * sin(-rotSpeed) + dirY * cos(-rotSpeed);
    float oldPlaneX = planeX;
    planeX = planeX * cos(-rotSpeed) - planeY * sin(-rotSpeed);
    planeY = oldPlaneX * sin(-rotSpeed) + planeY * cos(-rotSpeed);
  }
  if (btnRight) {
    float oldDirX = dirX;
    dirX = dirX * cos(rotSpeed) - dirY * sin(rotSpeed);
    dirY = oldDirX * sin(rotSpeed) + dirY * cos(rotSpeed);
    float oldPlaneX = planeX;
    planeX = planeX * cos(rotSpeed) - planeY * sin(rotSpeed);
    planeY = oldPlaneX * sin(rotSpeed) + planeY * cos(rotSpeed);
  }
}