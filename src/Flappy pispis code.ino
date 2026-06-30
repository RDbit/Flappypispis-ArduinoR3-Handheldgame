#include <Arduino.h>
#include <U8g2lib.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

// Buttons
const int buttonRight = 13;
const int buttonUp = 9;

// Buzzer
const int buzzer = 8;

// Bird & game variables
int birdY = 32;
int velocity = 0;
const int gravity = 1;
const int jumpStrength = -4;

// Pipes
const int numPipes = 10;
int pipeX[numPipes];
int pipeGapY[numPipes];
const int pipeGapSize = 16;
const int pipeSpacing = 12;
const int pipeWidth = 10;

// Score
unsigned long score = 0;

// Game state
bool gameRunning = false;
bool gameOver = false;

// ===== Notes for buzzer =====
#define NOTE_C4  262
#define NOTE_E4  330
#define NOTE_G4  392
#define NOTE_C5  523
#define NOTE_A4  440
#define NOTE_D4  294
#define NOTE_F4  349

// ===== Theme song (loop) =====
int melody[] = { NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5 };
int noteDurations[] = { 200, 200, 200, 400 }; // ms
const int melodyLength = 4;

int currentNote = 0;
unsigned long lastNoteTime = 0;

// RIGHT button check with debounce
bool rightPressed() {
  if (digitalRead(buttonRight) == LOW) {
    delay(150);
    while(digitalRead(buttonRight) == LOW); // wait release
    return true;
  }
  return false;
}

// Jump sound
void playJumpSound() {
  tone(buzzer, NOTE_A4, 80);
}

// Score sound
void beepBuzzer() {
  tone(buzzer, 1000, 100);
}

// Game Over tune
void playGameOverTune() {
  tone(buzzer, NOTE_C5, 200); delay(220);
  tone(buzzer, NOTE_G4, 200); delay(220);
  tone(buzzer, NOTE_E4, 300); delay(320);
  tone(buzzer, NOTE_C4, 500); delay(520);
  noTone(buzzer);
}

// Non-blocking theme loop
void playThemeLoop() {
  if (millis() - lastNoteTime > noteDurations[currentNote]) {
    lastNoteTime = millis();
    tone(buzzer, melody[currentNote], noteDurations[currentNote] - 20);
    currentNote++;
    if (currentNote >= melodyLength) currentNote = 0; // loop
  }
}

void setup() {
  pinMode(buttonRight, INPUT_PULLUP);
  pinMode(buttonUp, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  u8g2.begin();

  // Initialize pipes
  for(int i=0;i<numPipes;i++){
    pipeX[i] = 128 + i*(pipeWidth + pipeSpacing);
    pipeGapY[i] = random(pipeGapSize+2, 64 - pipeGapSize-2);
  }
}

void drawBird() {
  u8g2.drawCircle(20, birdY, 3, U8G2_DRAW_ALL);
  u8g2.drawDisc(22, birdY-1, 1, U8G2_DRAW_ALL);
  u8g2.drawDisc(18, birdY-2, 1, U8G2_DRAW_ALL);
  u8g2.drawPixel(19, birdY-1);
}

void resetGame() {
  birdY = 32;
  velocity = 0;
  score = 0;
  gameRunning = true;
  gameOver = false;
  noTone(buzzer);

  for(int i=0;i<numPipes;i++){
    pipeX[i] = 128 + i*(pipeWidth + pipeSpacing);
    pipeGapY[i] = random(pipeGapSize+2, 64-pipeGapSize-2);
  }
}

void loop() {
  u8g2.firstPage();
  do{
    // ===== START MENU =====
    if(!gameRunning && !gameOver){
      playThemeLoop(); // non-blocking looped music

      u8g2.setFont(u8g2_font_7x13B_tr);
      u8g2.setCursor(15,20);
      u8g2.print("FLAPPY PISPIS");
      u8g2.setFont(u8g2_font_6x12_tr);
      u8g2.setCursor(25,38);
      u8g2.print("by Ardzs");
      u8g2.setFont(u8g2_font_5x8_tr);
      u8g2.setCursor(20,55);
      u8g2.print("Press RIGHT to Play");

      if(rightPressed()) resetGame();
    }

    // ===== GAME OVER MENU =====
    else if(!gameRunning && gameOver){
      playGameOverTune();

      u8g2.setFont(u8g2_font_7x13B_tr);
      u8g2.setCursor(30, 20);
      u8g2.print("GAME OVER!");
      u8g2.setFont(u8g2_font_6x12_tr);
      u8g2.setCursor(40, 38);
      u8g2.print("Score: "); u8g2.print(score);
      u8g2.setFont(u8g2_font_5x8_tr);
      u8g2.setCursor(20, 55);
      u8g2.print("Press RIGHT to Restart");

      if(rightPressed()){
        gameOver = false;
        gameRunning = false;
        currentNote = 0; // reset theme
      }
    }

    // ===== GAMEPLAY =====
    else if(gameRunning){
      if(digitalRead(buttonUp)==LOW) {
        velocity = jumpStrength;
        playJumpSound();
      }
      velocity += gravity;
      birdY += velocity;

      for(int i=0;i<numPipes;i++){
        pipeX[i] -= 2;
        if(pipeX[i]<-pipeWidth){
          int maxX = pipeX[0];
          for(int j=1;j<numPipes;j++) if(pipeX[j]>maxX) maxX = pipeX[j];
          pipeX[i] = maxX + pipeWidth + pipeSpacing;
          pipeGapY[i] = random(pipeGapSize+2, 64-pipeGapSize-2);
          score++;
          beepBuzzer();
        }
        u8g2.drawBox(pipeX[i], 0, pipeWidth, pipeGapY[i]-pipeGapSize);
        u8g2.drawBox(pipeX[i], pipeGapY[i]+pipeGapSize, pipeWidth, 64-(pipeGapY[i]+pipeGapSize));
      }

      for(int i=0;i<numPipes;i++){
        if((pipeX[i]<24 && pipeX[i]>14 &&
           (birdY < pipeGapY[i]-pipeGapSize || birdY > pipeGapY[i]+pipeGapSize)) || 
           birdY <0 || birdY>63){
          gameRunning = false;
          gameOver = true;
        }
      }

      drawBird();

      u8g2.setFont(u8g2_font_5x8_tr);
      u8g2.setCursor(2,10);
      u8g2.print("Score:");
      char buf[12];
      sprintf(buf,"%lu",score);
      u8g2.setCursor(40,10);
      u8g2.print(buf);
    }

  } while(u8g2.nextPage());

  delay(50);
}
