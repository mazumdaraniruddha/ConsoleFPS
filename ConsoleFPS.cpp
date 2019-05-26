#include <iostream>
#include <Windows.h>
#include <chrono>
#include <vector>
#include <algorithm>
#include <cstdio>

using namespace std;

int mScreenWidth = 120;
int mScreenHeight = 40;

float fPlayerX = 8.0f;
float fPlayerY = 8.0f;
float fPlayerAngle = 0.0f;

int mMapWidth = 16;
int mMapHeight = 16;

float fFOV = 3.14159 / 4.0;
float fDepth = 16.0f;

int main() {
	wchar_t* screen = new wchar_t[mScreenHeight * mScreenWidth];
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;

	// Game Map
	wstring map;
	map += L"################";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..........#...#";
	map += L"#..........#...#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#.......########";
	map += L"#..............#";
	map += L"#..............#";
	map += L"################";

	auto tp1 = chrono::system_clock::now();
	auto tp2 = chrono::system_clock::now();

	bool bPlayerWantsToExit = false;

	// Game Loop
	while (!bPlayerWantsToExit) {
		// Calculate the elapsed time
		tp2 = chrono::system_clock::now();
		chrono::duration<float> elaspedTime = tp2 - tp1;
		tp1 = tp2;
		float fElapsedTime = elaspedTime.count();

		// Update player angle and position on user input
		if (GetAsyncKeyState((unsigned short) 'A') & 0x8000) {
			fPlayerAngle -= 0.5f * fElapsedTime;
		}
		if (GetAsyncKeyState((unsigned short) 'D') & 0x8000) {
			fPlayerAngle += 0.5f * fElapsedTime;
		}
		if (GetAsyncKeyState((unsigned short) 'W') & 0x8000) {
			fPlayerX += sinf(fPlayerAngle) * 5.0f * fElapsedTime;
			fPlayerY += cosf(fPlayerAngle) * 5.0f * fElapsedTime;
			// Detect collision with wall
			if (map[(int) fPlayerY * mMapWidth + (int) fPlayerX] == '#') {
				fPlayerX -= sinf(fPlayerAngle) * 5.0f * fElapsedTime;
				fPlayerY -= cosf(fPlayerAngle) * 5.0f * fElapsedTime;
			}
		}
		if (GetAsyncKeyState((unsigned short) 'S') & 0x8000) {
			fPlayerX -= sinf(fPlayerAngle) * 5.0f * fElapsedTime;
			fPlayerY -= cosf(fPlayerAngle) * 5.0f * fElapsedTime;
			// Detect collision with wall
			if (map[(int)fPlayerY * mMapWidth + (int)fPlayerX] == '#') {
				fPlayerX += sinf(fPlayerAngle) * 5.0f * fElapsedTime;
				fPlayerY += cosf(fPlayerAngle) * 5.0f * fElapsedTime;
			}
		}
		if (GetAsyncKeyState((unsigned short) ' ') & 0x8000) {
			bPlayerWantsToExit = true;
		}

		// Traverse all the x lines/rays (player's horizontal rays/each columns going out to the world)
		for (int x = 0; x < mScreenWidth; x++) {
			// For each column, calculate the ray angle into the world space
			float fRayAngle = (fPlayerAngle - fFOV / 2.0f) + ((float) x / (float) mScreenWidth) * fFOV;
			
			float fDistanceToWall = 0;
			bool bHitWall = false;
			bool bBoundary = false;

			// Unit vector for ray in Player space
			float fEyeX = sinf(fRayAngle);
			float fEyeY = cosf(fRayAngle);

			while (!bHitWall && fDistanceToWall < fDepth) {
				fDistanceToWall += 0.1f;
				
				int mTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
				int mTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

				// Test if ray is out of bounds?
				if (mTestX < 0 || mTestX >= mMapWidth || mTestY < 0 || mTestY >= mMapHeight) {
					bHitWall = true;
					fDistanceToWall = fDepth;
				} else {
					if (map[mTestY * mMapWidth + mTestX] == '#') {
						bHitWall = true;
						// Distance and dot product of the 2 vectors
						vector<pair<float, float>> p;
						for (int tx = 0; tx < 3; tx++) {
							for (int ty = 0; ty < 3; ty++) {
								float vy = (float)mTestY + ty - fPlayerY;
								float vx = (float)mTestX + tx - fPlayerX;
								float d = sqrt(vx*vx + vy*vy);
								float dotProduct = (fEyeX * vx / d) + (fEyeY * vy / d);
								p.push_back(make_pair(d, dotProduct));
							}
						}
						sort(p.begin(), p.end(), 
							[](const pair<float, float> &left, const pair<float, float> &right) {
							return left.first < right.first;
						});

						float fBound = 0.01;
						if (acos(p.at(0).second) < fBound) {
							bBoundary = true;
						}
						if (acos(p.at(1).second) < fBound) {
							bBoundary = true;
						}
						if (acos(p.at(2).second) < fBound) {
							bBoundary = true;
						}
					}
				}
			}

			// Calulate floow and ceiling
			int mCeiling = (float) (mScreenHeight / 2.0f) - mScreenHeight / ((float) fDistanceToWall);
			int mFloor = mScreenHeight - mCeiling;

			// Shade the wall
			short sShade = ' ';
			if (fDistanceToWall <= fDepth / 4.0f) {
				sShade = 0x2588;
			} else if (fDistanceToWall <= fDepth / 3.0f) {
				sShade = 0x2593;
			} else if (fDistanceToWall <= fDepth / 2.0f) {
				sShade = 0x2592;
			} else if (fDistanceToWall <= fDepth) {
				sShade = 0x2591;
			} else {
				sShade = ' ';
			}
			if (bBoundary) {
				sShade = ' ';
			}

			for (int y = 0; y < mScreenHeight; y++) {
				if (y < mCeiling) {
					// Draw celing
					screen[y * mScreenWidth + x] = ' ';
				} else if (y > mCeiling && y <= mFloor) {
					// Draw wall
					screen[y * mScreenWidth + x] = sShade;
				} else {
					// Draw floor
					float floorShadeFactor = 1.0f - (((float) y - mScreenHeight / 2.0f) / ((float) mScreenHeight / 2.0f));
					short sShadeFloor = ' ';
					
					if (floorShadeFactor < 0.25) {
						sShadeFloor = '#';
					} else if (floorShadeFactor < 0.5) {
						sShadeFloor = 'x';
					} if (floorShadeFactor < 0.75) {
						sShadeFloor = '.';
					} if (floorShadeFactor < 0.9) {
						sShadeFloor = '_';
					} else {
						sShadeFloor = ' ';
					}
					screen[y * mScreenWidth + x] = sShadeFloor;
				}
			}
		}

		// Display stats
		swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f, FPS=%3.2f", fPlayerX, fPlayerY, fPlayerAngle, 1/ fElapsedTime);

		// Minimap
		for (int x = 0; x < mMapWidth; x++) {
			for (int y = 0; y < mMapHeight; y++) {
				screen[(y + 1) * mScreenWidth + x] = map[y * mMapWidth + x];
			}
		}
		// Draw player in minimap
		screen[((int)fPlayerY + 1) * mScreenWidth + (int)fPlayerX] = '*';

		screen[mScreenWidth * mScreenHeight - 1] = '\0';
		WriteConsoleOutputCharacter(hConsole, screen, mScreenWidth * mScreenHeight, { 0, 0 }, &dwBytesWritten);
	}

	return 0;
}
