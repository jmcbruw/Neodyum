#include <Windows.h>
#include <d2d1.h>
#include <wincodec.h>
#include <dwrite.h>
#include <chrono>
#include <vector>
#include <map>
#include <limits>
#include <time.h>
#include <stdlib.h>
#include <thread>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "dwrite.lib")


// Contains all filenames necessary for the game to run. Condensed into variables to improve readability in later code.
LPCWSTR player1 = L"C:\\Users\\receiving\\source\\repos\\Shoot Game\\Shoot Game\\Sprites\\Ship\\Ship1.png";
LPCWSTR player2 = L"C:\\Users\\receiving\\source\\repos\\Shoot Game\\Shoot Game\\Sprites\\Ship\\Ship2.png";
LPCWSTR enemy1_1 = L"C:\\Users\\receiving\\source\\repos\\Shoot Game\\Shoot Game\\Sprites\\Enemies\\Enemy1_1.png";
LPCWSTR enemy1_2 = L"C:\\Users\\receiving\\source\\repos\\Shoot Game\\Shoot Game\\Sprites\\Enemies\\Enemy1_2.png";
LPCWSTR background1 = L"C:\\Users\\receiving\\source\\repos\\Shoot Game\\Shoot Game\\Sprites\\Background\\Background1.png";
LPCWSTR playerBasicShot1 = L"C:\\Users\\receiving\\source\\repos\\Shoot Game\\Shoot Game\\Sprites\\Shots\\PlayerBasicShot1.png";

// ID2D1Factory interface is the starting point for using Direct2D; it's what you use to create other Direct2D resources
ID2D1Factory* pD2DFactory = NULL;

// ID2D1HwndRenderTarget represents an object that can receive drawing commands.
ID2D1HwndRenderTarget* pRenderTarget = NULL;

// ID2D1Bitmap represents a bitmap that has been bound to an ID2D1RenderTarget
std::map<LPCWSTR, ID2D1Bitmap*> pBitmaps;

// IDWriteFactory interface is the root factory interface for all DirectWrite objects, used for text
IDWriteFactory* pDWriteFactory = NULL;

// IDWriteTextFormat interface describes the font and paragraph properties used to format text, and it describes locale information.
IDWriteTextFormat* pTextFormat = NULL;

// System screen horizontal and vertical resolution
int sysScreenX = GetSystemMetrics(SM_CXSCREEN);
int sysScreenY = GetSystemMetrics(SM_CYSCREEN);

// Total number of frames
unsigned long long int frameCount = 0;

// Indexes the last time the fps was updated
std::chrono::time_point<std::chrono::steady_clock> lastUpdate;

// framerate
double fps = 0.0;

// ID2D1SolidColorBrush paints an area with a solid color.
ID2D1SolidColorBrush* pBrush = NULL;

// Index files
std::vector<LPCWSTR> filePaths;

// Use the steady_clock for timing to prevent issues with time changes
std::chrono::steady_clock::time_point lastMoveTime = std::chrono::steady_clock::now();

double moveInterval = 1 / 500; // Move every 0.002 seconds


// contains booleans for key presses
struct
{
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool space = false;
    bool lShift = false;
    bool escape = false;
} keys;

void GetDirectionalInput(double& xDirection, double& yDirection, bool keyRight, bool keyLeft, bool keyDown, bool keyUp)
{
    xDirection = (keyRight ? 1 : 0) - (keyLeft ? 1 : 0);
    yDirection = (keyDown ? 1 : 0) - (keyUp ? 1 : 0);
}

class Object
{
public:
    double xPosition = sysScreenX / 2;
    double yPosition = sysScreenY / 2;
    LPCWSTR fileName = nullptr;
    LPCWSTR lastfilepath = nullptr;
    LPCWSTR secondlastfilepath = nullptr;
    double lastFrameXDirection = 0;
    double lastFrameYDirection = 0;
    double lastXMoveDirection = 0;
    double lastYMoveDirection = 0;
    std::chrono::steady_clock::time_point lastMoveTime = std::chrono::steady_clock::now();
    int framesWalked2 = 0;
    bool fileNameChanged = false;
    int state = 3;
    int count = 0;
    D2D1_RECT_F hurtbox;
    double scale;

    double lastXDirection2 = 0;
    double lastYDirection2 = 0;

    void WriteFileName(LPCWSTR file_Name)
    {
        fileName = file_Name;
        fileNameChanged = true; // Flag that the filename has changed
    }

    void DestroyObject()
    {
        fileName = nullptr;
        fileNameChanged = true;
    }
};

class Enemy : public Object
{
public:

    int hp = 10;
    int maxHP = 10;
    bool damageTaken = false;
    bool alreadyHit = false;
    bool inHitstun = false;
    std::chrono::nanoseconds hitStunTime;
    std::chrono::steady_clock::time_point hitStunStartTime = std::chrono::steady_clock::now();

    Enemy()
    {
        xPosition = rand() % 1920;
        yPosition = rand() % 1080;
        fileName = enemy1_1;
        fileNameChanged = true;
        SetEnemyHurtBox();
    }

    void SetEnemyHurtBox()
    {
        // Get the bitmap for the current object frame
        ID2D1Bitmap* pBitmap = pBitmaps[fileName];
        if (pBitmap)
        {
            D2D1_SIZE_F size = pBitmap->GetSize();
            hurtbox = D2D1::RectF(xPosition + 10, yPosition + 10,
                size.width + xPosition - 20, size.height + yPosition);
        }

    }

    // Displaces the Object
    void MoveEnemy(double xDirection, double yDirection)
    {
        xPosition += xDirection;
        yPosition += yDirection;

        lastXMoveDirection = xDirection;
        lastYMoveDirection = yDirection;
        SetEnemyHurtBox();
    }

    // Displaces the Object
    void DisplaceEnemy(double xDirection, double yDirection)
    {
        xPosition += xDirection;
        yPosition += yDirection;
        SetEnemyHurtBox();
    }

    void EnemyMoveAnimation()
    {
        if (std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - lastMoveTime) >= std::chrono::nanoseconds(166666666))
        {
            if (fileName == enemy1_1)
            {
                fileName = enemy1_2;
            }
            else
            {
                fileName = enemy1_1;
            }
            lastMoveTime = std::chrono::steady_clock::now();
            fileNameChanged = true;
        }
    }
};

class Player : public Object
{
public:

    int level = 1;
    double exp = 0;
    double levelup = 100;

    double HP = 100;
    double maxHP = 100;
    double MP = 50;
    double maxMP = 50;
    int strength = 9;
    int dexterity = 10;
    int intelligence = 5;
    int wisdom = 7;
    int defense = 5;
    int magicDefense = 3;
    int trueDefense = 1;
    int speed = 10;
    int luck = 8;
    bool shotCooldown = true;
    // Calculate the rotation angle based on player's movement direction
    double angle = atan2(static_cast<double>(lastYMoveDirection), static_cast<double>(lastXMoveDirection));

    struct 
    {
        bool up = false;
        bool down = false;
        bool left = false;
        bool right = false;
        bool upright = false;
        bool downright = false;
        bool downleft = false;
        bool upleft = false;
    } facing;

    std::chrono::steady_clock::time_point lastBasicAttackFrame = std::chrono::steady_clock::now();
    std::chrono::nanoseconds basicAttackFrameIncrements = std::chrono::nanoseconds(16666666 / 2);
    std::chrono::nanoseconds basicAttackStartLag = std::chrono::nanoseconds(66666666);
    std::chrono::nanoseconds basicAttackEndLag = std::chrono::nanoseconds(133333333);
    std::chrono::nanoseconds hitLag = std::chrono::nanoseconds(133333333);
    std::chrono::nanoseconds moveAnimationInterval = std::chrono::nanoseconds(166666666);

    std::chrono::steady_clock::time_point timeOfLastShot = std::chrono::steady_clock::now();
    std::chrono::nanoseconds timeSinceLastShot = std::chrono::nanoseconds(16666666 / 2);
    bool isBasicAttacking = false;
    LPCWSTR weaponFileName;
    double weaponXPosition = xPosition;
    double weaponYPosition = yPosition;
    int basicAttackFrameThresholds = 1;
    D2D1_RECT_F hitbox;
    bool madeContact = false;
    bool frameAdvanced = false;
    LPCWSTR hpBarFileName;

    Player()
    {
        fileName = player1;
        fileNameChanged = true;
        scale = 5;
        SetPlayerHurtBox();
    }

    void PlayerIdle()
    {
        if (std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - lastMoveTime) >= moveAnimationInterval)
        {
            if (fileName == player1)
            {
                fileName = player2;
            }
            else
            {
                fileName = player1;
            }
            lastMoveTime = std::chrono::steady_clock::now(); // reset the frame counter
            fileNameChanged = true; // Flag that the filename has changed
        }
        SetPlayerHurtBox();
    }

    void PlayerMoveAnimation(double xDirection, double yDirection)
    {
        if (std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - lastMoveTime) >= moveAnimationInterval)
        {
            if (fileName == player1)
            {
                fileName = player2;
            }
            else
            {
                fileName = player1;
            }
            lastMoveTime = std::chrono::steady_clock::now(); // reset the frame counter
            fileNameChanged = true; // Flag that the filename has changed
        }
    }

    void BasicShot(std::vector<Enemy>& enemies)
    {
        shotCooldown = false;
        timeOfLastShot = std::chrono::steady_clock::now();
        timeSinceLastShot = std::chrono::steady_clock::now() - timeOfLastShot;
    }

    // Sets the hurtbox for the object. This is how the game knows if the player has been hit or is touching something.
    void SetPlayerHurtBox()
    {
        // Get the bitmap for the current object frame
        ID2D1Bitmap* pBitmap = pBitmaps[fileName];
        if (pBitmap)
        {
            D2D1_SIZE_F size = pBitmap->GetSize();
            hurtbox = D2D1::RectF(xPosition + 30, yPosition + 10,
                size.width + xPosition - 30, size.height + yPosition - 15);
        }

    }

    // Displaces the Object
    void MovePlayer(double xDirection, double yDirection)
    {
        xPosition += xDirection;
        yPosition += yDirection;

        lastXMoveDirection = xDirection;
        lastYMoveDirection = yDirection;
        angle = -1 * atan2(static_cast<double>(lastYMoveDirection), static_cast<double>(lastXMoveDirection));

        SetPlayerHurtBox();
    }

    void SetHitBox()
    {
        // Get the bitmap for the current object frame
        ID2D1Bitmap* pBitmap = pBitmaps[weaponFileName];
        if (pBitmap)
        {
            D2D1_SIZE_F size = pBitmap->GetSize();
            hitbox = D2D1::RectF(weaponXPosition, weaponYPosition,
                size.width + weaponXPosition, size.height + weaponYPosition);
        }
    }

    void RemoveHitBox()
    {
        hitbox.right = hitbox.left = hitbox.top = hitbox.bottom = -1;
    }

};

class Shot : public Object
{
public: 

    double xDirection;
    double yDirection;
    bool newBullet = true;
    void InitializeShot(Player player)
    {
        if (player.lastXMoveDirection >= 0)
        {
            xDirection = ceil(player.lastXMoveDirection);
        }
        else
        {
            xDirection = floor(player.lastXMoveDirection);
        }
        if (player.lastYMoveDirection >= 0)
        {
            yDirection = ceil(player.lastYMoveDirection);
        }
        else
        {
            yDirection = floor(player.lastYMoveDirection);
        }
        scale = 3;

        fileName = playerBasicShot1;

        // Get the bitmap for the current player frame from the map
        ID2D1Bitmap* playerBitmap = pBitmaps[player.fileName];
        D2D1_SIZE_F shipSize = playerBitmap->GetSize();
        // Get the bitmap for the current player frame from the map
        ID2D1Bitmap* shotBitmap = pBitmaps[fileName];
        D2D1_SIZE_F shotSize = shotBitmap->GetSize();

        if (player.facing.up == true)
        {
            xPosition = player.xPosition + (player.scale * shipSize.width / 2) - (scale * shotSize.width / 2);
            yPosition = player.yPosition - (scale * shotSize.height);
        }
        else if (player.facing.down == true)
        {
            xPosition = player.xPosition + (player.scale * shipSize.width / 2) - (scale * shotSize.width / 2);
            yPosition = player.yPosition + (player.scale * shipSize.height);
        }
        else if (player.facing.left == true)
        {
            xPosition = player.xPosition - (scale * shotSize.width) - scale * ((shotSize.height - shotSize.width) /2);
            yPosition = player.yPosition + (player.scale * shipSize.width / 2) - (scale * shotSize.height / 2);
        }
        else if (player.facing.right == true)
        {
            xPosition = player.xPosition + (player.scale * shipSize.width) + scale * ((shotSize.height - shotSize.width) / 2);
            yPosition = player.yPosition + (player.scale * shipSize.width / 2) - (scale * shotSize.height / 2);
        }

        else if (player.facing.upright == true)
        {
            xPosition = 1409/*player.xPosition + (player.scale * shipSize.width / 2) + ((player.scale * shipSize.width / 2) * cos(player.angle)) - 3*/;
            /*xPosition = player.xPosition + (player.scale * shipSize.width / 2) + (player.scale * 1 * cos(player.angle) * shipSize.width);*/
            double x = cos(player.angle);
            yPosition = 696/*player.yPosition + (player.scale * (shipSize.height / 2) * (1 - sin(player.angle)) - 47)*/;
        }
        else if (player.facing.downright == true)
        {
            xPosition = 1409/*player.xPosition + (player.scale * shipSize.width / 2) + ((player.scale * shipSize.width / 2) * cos(player.angle))*/;
            yPosition = 851/*player.yPosition + (player.scale * shipSize.height) - 3 * scale*/;
        }
        else if (player.facing.downleft == true)
        {
            xPosition = 1261/*player.xPosition - (scale * shotSize.width) + 2 * scale*/;
            yPosition = 844/*player.yPosition + (player.scale * shipSize.width / 2) - (scale * shotSize.height / 2)*/;
        }
        else if (player.facing.upleft == true)
        {
            xPosition = 1261/*player.xPosition + (player.scale * shipSize.width) - 2 * scale*/;
            yPosition = 696/*player.yPosition + (player.scale * shipSize.width / 2) - (scale * shotSize.height / 2)*/;
        }
    }
    void DisplaceShot()
    {
        xPosition += 1 * xDirection;
        yPosition += 1 * yDirection;
    }
};

/*class Enemy : public Object
{
    int leafEnemyWalkFrames = 0;
    void MoveObject(double xDir, double yDir)
    {
        xPosition += xDir;
        yPosition += yDir;
    }
};*/

void ApplyPlayerDirectionalInput(Player& player, std::chrono::duration<float> elapsedTime, std::chrono::steady_clock::time_point currentFrameTime, double xDir, double yDir)
{
    if ((xDir != 0 || yDir != 0) && player.hurtbox.left >= 0 && player.hurtbox.top >= 0
        && player.hurtbox.right <= sysScreenX && player.hurtbox.bottom <= sysScreenY)
    {
        player.MovePlayer(xDir, yDir);
        if (player.hurtbox.left >= 0 && player.hurtbox.top >= 0
            && player.hurtbox.right <= sysScreenX && player.hurtbox.bottom <= sysScreenY)
        {
            player.PlayerMoveAnimation(3 * xDir, 3 * yDir);
        }
        else
        {
            player.PlayerIdle();
        }

        // If the player was moved out of bounds, snaps them back in bounds
        if (player.hurtbox.left <= 0)
        {
            player.MovePlayer(0 - player.hurtbox.left, 0);
        }
        if (player.hurtbox.top <= 0)
        {
            player.MovePlayer(0, 0 - player.hurtbox.top);
        }
        if (player.hurtbox.right >= sysScreenX)
        {
            player.MovePlayer(sysScreenX - player.hurtbox.right, 0);
        }
        if (player.hurtbox.bottom >= sysScreenY)
        {
            player.MovePlayer(0, sysScreenY - player.hurtbox.bottom);
        }
    }
    else
    {
        player.PlayerIdle();
    }
}

void ApplyEnemyDirectionalInput(Enemy& enemy, double xDir, double yDir)
{
    enemy.SetEnemyHurtBox();
    if (xDir != 0 || yDir != 0)
    {
        enemy.MoveEnemy(xDir, yDir);

        // If the enemy was moved out of bounds, snaps them back in bounds
        if (enemy.hurtbox.left <= 0)
        {
            enemy.DisplaceEnemy(0 - enemy.hurtbox.left, 0);
            enemy.lastXMoveDirection *= -1;
        }
        if (enemy.hurtbox.top <= 0)
        {
            enemy.DisplaceEnemy(0, 0 - enemy.hurtbox.top);
            enemy.lastYMoveDirection *= -1;
        }
        if (enemy.hurtbox.right >= sysScreenX)
        {
            enemy.DisplaceEnemy(sysScreenX - enemy.hurtbox.right, 0);
            enemy.lastXMoveDirection *= -1;
        }
        if (enemy.hurtbox.bottom >= sysScreenY)
        {
            enemy.DisplaceEnemy(0, sysScreenY - enemy.hurtbox.bottom);
            enemy.lastYMoveDirection *= -1;
        }
    }

    if (enemy.hurtbox.left == 0 && enemy.hurtbox.top == 0
        && enemy.hurtbox.right == 0 && enemy.hurtbox.bottom == 0)
    {
        enemy.framesWalked2 = 0;
    }
    else
    {
        enemy.framesWalked2++;
    }

    enemy.EnemyMoveAnimation();
}

void LoadSpriteData(std::vector<Object>& spriteData)
{
    spriteData.reserve(6);

    //down stand and move
    spriteData.emplace_back();
    spriteData.at(0).WriteFileName(player1);
    spriteData.emplace_back();
    spriteData.at(1).WriteFileName(player2);
    spriteData.emplace_back();
    spriteData.at(2).WriteFileName(enemy1_1);

    // up stand and move
    spriteData.emplace_back();
    spriteData.at(3).WriteFileName(enemy1_2);
    spriteData.emplace_back();
    spriteData.at(4).WriteFileName(background1);
    spriteData.emplace_back();
    spriteData.at(5).WriteFileName(playerBasicShot1);
   
}

//void CreateDeviceResources(HWND hWnd, Object objects)
//{
//    HRESULT hr = S_OK;
//
//    if (!pRenderTarget)
//    {
//        RECT rc;
//        GetClientRect(hWnd, &rc);
//
//        D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
//
//        hr = pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hWnd, size), &pRenderTarget);
//    }
//
//    if (FAILED(hr))
//    {
//        return;
//    }
//
//    IWICImagingFactory* pWICFactory = NULL;
//    hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, reinterpret_cast<void**>(&pWICFactory));
//
//    if (FAILED(hr))
//    {
//        return;
//    }
//    if (objects.fileNameChanged || pBitmaps.find(objects.fileName) == pBitmaps.end())
//    {
//        IWICBitmapDecoder* pDecoder = NULL;
//        hr = pWICFactory->CreateDecoderFromFilename(objects.fileName,
//            NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
//
//        if (FAILED(hr))
//        {
//            pWICFactory->Release();
//            return;
//        }
//
//        IWICBitmapFrameDecode* pSource = NULL;
//        hr = pDecoder->GetFrame(0, &pSource);
//
//        if (FAILED(hr))
//        {
//            pDecoder->Release();
//            pWICFactory->Release();
//            return;
//        }
//
//        IWICFormatConverter* pConverter = NULL;
//        hr = pWICFactory->CreateFormatConverter(&pConverter);
//
//        if (FAILED(hr))
//        {
//            pSource->Release();
//            pDecoder->Release();
//            pWICFactory->Release();
//            return;
//        }
//
//        hr = pConverter->Initialize(pSource, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeMedianCut);
//
//        if (FAILED(hr))
//        {
//            pConverter->Release();
//            pSource->Release();
//            pDecoder->Release();
//            pWICFactory->Release();
//            return;
//        }
//
//        ID2D1Bitmap* pBitmap = NULL; // Create new bitmap for each file
//        hr = pRenderTarget->CreateBitmapFromWicBitmap(pConverter, NULL, &pBitmap);
//
//        if (SUCCEEDED(hr))
//        {
//            pBitmaps[objects.fileName] = pBitmap; // Add the new bitmap to our bitmap map
//            objects.fileNameChanged = false; // Reset the flag
//        }
//
//        // Release resources for each file
//        pConverter->Release();
//        pSource->Release();
//        pDecoder->Release();
//    }
//
//    // Release the WIC factory after processing all files
//    pWICFactory->Release();
//
//    // Create DirectWrite factory and text format for rendering FPS counter
//    hr = DWriteCreateFactory(
//        DWRITE_FACTORY_TYPE_SHARED,
//        __uuidof(IDWriteFactory),
//        reinterpret_cast<IUnknown**>(&pDWriteFactory)
//    );
//    if (SUCCEEDED(hr)) {
//        hr = pDWriteFactory->CreateTextFormat(
//            L"Arial",
//            NULL,
//            DWRITE_FONT_WEIGHT_NORMAL,
//            DWRITE_FONT_STYLE_NORMAL,
//            DWRITE_FONT_STRETCH_NORMAL,
//            20.0f,
//            L"en-us",
//            &pTextFormat
//        );
//    }
//
//    // Create the brush for text
//    if (SUCCEEDED(hr)) {
//        hr = pRenderTarget->CreateSolidColorBrush(
//            D2D1::ColorF(D2D1::ColorF::White),
//            &pBrush
//        );
//    }
//}
 
void CreateDeviceResources(HWND hWnd, std::vector<Object> spriteData)
{
    HRESULT hr = S_OK;

    if (!pRenderTarget)
    {
        RECT rc;
        GetClientRect(hWnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

        hr = pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hWnd, size), &pRenderTarget);
    }

    if (FAILED(hr))
    {
        return;
    }

    IWICImagingFactory* pWICFactory = NULL;
    hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, reinterpret_cast<void**>(&pWICFactory));

    if (FAILED(hr))
    {
        return;
    }

    // Iterating over each file in the objects vector
    for (int i = 0; i < spriteData.size(); i++)
    {
        if (spriteData.at(i).fileNameChanged || pBitmaps.find(spriteData.at(i).fileName) == pBitmaps.end())
        {
            IWICBitmapDecoder* pDecoder = NULL;
            hr = pWICFactory->CreateDecoderFromFilename(spriteData.at(i).fileName,
                NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);

            if (FAILED(hr))
            {
                pWICFactory->Release();
                return;
            }

            IWICBitmapFrameDecode* pSource = NULL;
            hr = pDecoder->GetFrame(0, &pSource);

            if (FAILED(hr))
            {
                pDecoder->Release();
                pWICFactory->Release();
                return;
            }

            IWICFormatConverter* pConverter = NULL;
            hr = pWICFactory->CreateFormatConverter(&pConverter);

            if (FAILED(hr))
            {
                pSource->Release();
                pDecoder->Release();
                pWICFactory->Release();
                return;
            }

            hr = pConverter->Initialize(pSource, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeMedianCut);

            if (FAILED(hr))
            {
                pConverter->Release();
                pSource->Release();
                pDecoder->Release();
                pWICFactory->Release();
                return;
            }

            ID2D1Bitmap* pBitmap = NULL; // Create new bitmap for each file
            hr = pRenderTarget->CreateBitmapFromWicBitmap(pConverter, NULL, &pBitmap);

            if (SUCCEEDED(hr))
            {
                pBitmaps[spriteData.at(i).fileName] = pBitmap; // Add the new bitmap to our bitmap map
                spriteData.at(i).fileNameChanged = false; // Reset the flag
            }

            // Release resources for each file
            pConverter->Release();
            pSource->Release();
            pDecoder->Release();
        }
    }

    // Release the WIC factory after processing all files
    pWICFactory->Release();

    // Create DirectWrite factory and text format for rendering FPS counter
    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&pDWriteFactory)
    );
    if (SUCCEEDED(hr)) {
        hr = pDWriteFactory->CreateTextFormat(
            L"Arial",
            NULL,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            20.0f,
            L"en-us",
            &pTextFormat
        );
    }

    // Create the brush for text
    if (SUCCEEDED(hr)) {
        hr = pRenderTarget->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::White),
            &pBrush
        );
    }
}

void DiscardDeviceResources()
{
    // Release brush
    if (pBrush)
    {
        pBrush->Release();
        pBrush = NULL;
    }

    if (pRenderTarget)
    {
        pRenderTarget->Release();
        pRenderTarget = NULL;
    }
    for (auto& pair : pBitmaps)
    {
        ID2D1Bitmap* pBitmap = pair.second;
        if (pBitmap)
        {
            pBitmap->Release();
        }
    }
    pBitmaps.clear();
    if (pTextFormat)
    {
        pTextFormat->Release();
        pTextFormat = NULL;
    }
    if (pDWriteFactory)
    {
        pDWriteFactory->Release();
        pDWriteFactory = NULL;
    }
}

//void OnRender(HWND hWnd, Object objects)
//{
//    if (!pRenderTarget)
//    {
//        CreateDeviceResources(hWnd, objects);
//    }
//
//    if (pRenderTarget)
//    {
//        pRenderTarget->BeginDraw();
//
//        // Get the bitmap for the current object from the map
//        ID2D1Bitmap* pBitmap = pBitmaps[objects.fileName];
//
//        // Draw the bitmap
//        if (pBitmap)
//        {
//            D2D1_SIZE_F size = pBitmap->GetSize();
//            D2D1_RECT_F destRect = D2D1::RectF(objects.xPosition, objects.yPosition,
//                size.width + objects.xPosition, size.height + objects.yPosition);
//            pRenderTarget->DrawBitmap(pBitmap, destRect);
//        }
//
//        if (pTextFormat && pBrush)
//        {
//            // Compute FPS if a quarter of a second or more has passed since last update
//            auto now = std::chrono::steady_clock::now();
//            std::chrono::duration<double> elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate);
//            if (elapsed.count() >= 0.25) {
//                fps = frameCount / elapsed.count();
//                frameCount = 0;
//                lastUpdate = now;
//            }
//
//            // Render FPS counter
//            WCHAR buffer[32];
//            swprintf_s(buffer, L"FPS: %.2f", fps);
//            D2D1_RECT_F layoutRect = D2D1::RectF(sysScreenX - 100, 0, sysScreenX, 20);
//            pRenderTarget->DrawText(
//                buffer,
//                wcslen(buffer),
//                pTextFormat,
//                layoutRect,
//                pBrush
//            );
//        }
//
//        HRESULT hr = pRenderTarget->EndDraw();
//        if (hr == D2DERR_RECREATE_TARGET)
//        {
//            DiscardDeviceResources();
//        }
//    }
//}

void OnRender(HWND hWnd, std::vector<Object> spriteData, Player& player, std::vector<Enemy> enemies, 
    std::vector<Shot> shots)
{
    if (!pRenderTarget)
    {
        CreateDeviceResources(hWnd, spriteData);
    }

    // Define the center of the screen
    float screenCenterX = sysScreenX / 2.0f;
    float screenCenterY = sysScreenY / 2.0f;

    // Define the central region of the bitmap you want to render around the player's center
    D2D1_RECT_F position = D2D1::RectF(
        player.xPosition + 15.5 - 127.5,
        player.yPosition + 15.5 - 71.5,
        player.xPosition + 15.5 + 127.5,
        player.yPosition + 15.5 + 71.5
    );

    if (pRenderTarget)
    {
        pRenderTarget->BeginDraw();

        pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::DimGray));

        // Get Bitmap for background
        ID2D1Bitmap* background = pBitmaps[background1];

        // Draw background bitmap
        if (background)
        {
            D2D1_SIZE_F size = background->GetSize();
            D2D1_SIZE_F dpi;
            background->GetDpi(&dpi.width, &dpi.height);

            // Define the central region of the background you want to render
            D2D1_RECT_F srcRect = D2D1::RectF(
                player.xPosition - 128.0f,
                player.yPosition - 72.0f,
                player.xPosition + 128.0f,
                player.yPosition + 72.0f
            );

            // Destination rectangle (adjust this as needed, this will draw the central region to fill the entire target render area)
            D2D1_RECT_F destRect = D2D1::RectF(0, 0, sysScreenX, sysScreenY);

            pRenderTarget->DrawBitmap(background, destRect, 1.0F, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, srcRect);
        }

        // Get bitmap for and draw enemies
        for (int i = 0; i < enemies.size(); i++)
        {
            // Get the bitmap for the current player frame from the map
            ID2D1Bitmap* enemyFrameBitmap = pBitmaps[enemies.at(i).fileName];

            // Draw the Leaf enemy frame bitmap
            if (enemyFrameBitmap)
            {
                D2D1_SIZE_F size = enemyFrameBitmap->GetSize();
                int directionMultiplierX;
                int directionMultiplierY;
                if (enemies.at(i).lastXMoveDirection > 0)
                {
                    directionMultiplierX = 1;
                }
                if (enemies.at(i).lastYMoveDirection > 0)
                {
                    directionMultiplierY = 1;
                }
                if (enemies.at(i).lastXMoveDirection < 0)
                {
                    directionMultiplierX = -1;
                }
                if (enemies.at(i).lastYMoveDirection < 0)
                {
                    directionMultiplierY = -1;
                }
                if (enemies.at(i).lastXMoveDirection == 0)
                {
                    directionMultiplierX = 1;
                }
                if (enemies.at(i).lastYMoveDirection == 0)
                {
                    directionMultiplierY = 1;
                }
                D2D1_RECT_F destRect = D2D1::RectF(enemies.at(i).xPosition, enemies.at(i).yPosition,
                    (5 * size.width) + enemies.at(i).xPosition, (5 * size.height) + enemies.at(i).yPosition);

                // Calculate the center of the destRect for rotation
                D2D1_POINT_2F center = D2D1::Point2F(
                    (destRect.left + destRect.right) / 2.0f,
                    (destRect.top + destRect.bottom) / 2.0f
                );

                // Calculate the rotation angle based on player's movement direction
                double angle = atan2(static_cast<double>(enemies.at(i).lastYMoveDirection), static_cast<double>(enemies.at(i).lastXMoveDirection));

                // Convert radians to degrees if necessary (for debugging or if the Rotation function expects degrees)
                double angleDegrees = 90 + (angle * (180.0 / 3.14));

                // Create a rotation transform
                D2D1_MATRIX_3X2_F rotation = D2D1::Matrix3x2F::Rotation(angleDegrees, center); // If Rotation function expects degrees

                // Set the transform on the render target
                pRenderTarget->SetTransform(&rotation);

                // Draw the bitmap with the rotation applied
                pRenderTarget->DrawBitmap(enemyFrameBitmap, destRect, 1.0F, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);

                // Reset the transform to identity so other draw calls aren't affected
                pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
            }
        }




        // Get the bitmap for the current player frame from the map
        ID2D1Bitmap* playerFrameBitmap = pBitmaps[player.fileName];

        // Draw the player frame bitmap
        if (playerFrameBitmap)
        {
            D2D1_SIZE_F size = playerFrameBitmap->GetSize();
            int directionMultiplierX;
            int directionMultiplierY;
            if (player.lastXMoveDirection > 0)
            {
                directionMultiplierX = 1;
            }
            if (player.lastYMoveDirection > 0)
            {
                directionMultiplierY = 1;
            }
            if (player.lastXMoveDirection < 0)
            {
                directionMultiplierX = -1;
            }
            if (player.lastYMoveDirection < 0)
            {
                directionMultiplierY = -1;
            }
            if (player.lastXMoveDirection == 0)
            {
                directionMultiplierX = 1;
            }
            if (player.lastYMoveDirection == 0)
            {
                directionMultiplierY = 1;
            }
            // Adjusting the ship size to maintain a similar scale to your original code
            D2D1_RECT_F destRect = D2D1::RectF(
                screenCenterX - (size.width * 2.5f),
                screenCenterY - (size.height * 2.5f),
                screenCenterX + (size.width * 2.5f),
                screenCenterY + (size.height * 2.5f)
            );

            // Calculate the center of the destRect for rotation
            D2D1_POINT_2F center = D2D1::Point2F(
                (destRect.left + destRect.right) / 2.0f,
                (destRect.top + destRect.bottom) / 2.0f
            );

            // Calculate the rotation angle based on player's movement direction
            double angle = atan2(static_cast<double>(player.lastYMoveDirection), static_cast<double>(player.lastXMoveDirection));
            
            if (angle > -1.55 && angle < -0.05)
            {
                player.facing.upright = true;
                player.facing.up = false;
                player.facing.down = false;
                player.facing.left = false;
                player.facing.right = false;
                player.facing.upleft = false;
                player.facing.downleft = false;
                player.facing.downright = false;
            }
            else if (angle > 0.05 && angle < 1.55)
            {
                player.facing.downright = true;
                player.facing.down = false;
                player.facing.up = false;
                player.facing.left = false;
                player.facing.right = false;
                player.facing.upleft = false;
                player.facing.upright = false;
                player.facing.downleft = false;
            }
            else if (angle > 1.6 && angle < 3.1)
            {
                player.facing.downleft = true;
                player.facing.up = false;
                player.facing.right = false;
                player.facing.left = false;
                player.facing.down = false;
                player.facing.upleft = false;
                player.facing.upright = false;
                player.facing.downright = false;
            }
            else if (angle < -1.6)
            {
                player.facing.upleft = true;
                player.facing.down = false;
                player.facing.right = false;
                player.facing.up = false;
                player.facing.left = false;
                player.facing.upright = false;
                player.facing.downleft = false;
                player.facing.downright = false;
            }
            if (angle > -1.6 && angle < -1.55)
            {
                player.facing.up = true;
                player.facing.down = false;
                player.facing.right = false;
                player.facing.left = false;
                player.facing.upleft = false;
                player.facing.upright = false;
                player.facing.downleft = false;
                player.facing.downright = false;
            }
            else if (angle > -0.05 && angle < 0.05)
            {
                player.facing.right = true;
                player.facing.down = false;
                player.facing.up = false;
                player.facing.left = false;
                player.facing.upleft = false;
                player.facing.upright = false;
                player.facing.downleft = false;
                player.facing.downright = false;
            }
            else if (angle > 1.55 && angle < 1.6)
            {
                player.facing.down = true;
                player.facing.up = false;
                player.facing.right = false;
                player.facing.left = false;
                player.facing.upleft = false;
                player.facing.upright = false;
                player.facing.downleft = false;
                player.facing.downright = false;
            }
            else if (angle > 3.1 && angle < 3.2)
            {
                player.facing.left = true;
                player.facing.down = false;
                player.facing.right = false;
                player.facing.up = false;
                player.facing.upleft = false;
                player.facing.upright = false;
                player.facing.downleft = false;
                player.facing.downright = false;
            }
            
            
            // Convert radians to degrees if necessary (for debugging or if the Rotation function expects degrees)
            double angleDegrees = 90 + (angle * (180.0 / 3.14));

            // Create a rotation transform
            D2D1_MATRIX_3X2_F rotation = D2D1::Matrix3x2F::Rotation(angleDegrees, center); // If Rotation function expects degrees

            // Set the transform on the render target
            pRenderTarget->SetTransform(&rotation);

            // Draw the bitmap with the rotation applied
            pRenderTarget->DrawBitmap(playerFrameBitmap, destRect, 1.0F, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);

            // Reset the transform to identity so other draw calls aren't affected
            pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
        }


        for (int i = 0; i < shots.size(); i++)
        {
            // Get the bitmap for the shot from the map
            ID2D1Bitmap* shotBitmap = pBitmaps[shots.at(i).fileName];

            // Draw the shot bitmap
            if (shotBitmap)
            {
                D2D1_SIZE_F size = shotBitmap->GetSize();
                int directionMultiplierX;
                int directionMultiplierY;
                if (shots.at(i).lastXMoveDirection > 0)
                {
                    directionMultiplierX = 1;
                }
                if (shots.at(i).lastYMoveDirection > 0)
                {
                    directionMultiplierY = 1;
                }
                if (shots.at(i).lastXMoveDirection < 0)
                {
                    directionMultiplierX = -1;
                }
                if (shots.at(i).lastYMoveDirection < 0)
                {
                    directionMultiplierY = -1;
                }
                if (shots.at(i).lastXMoveDirection == 0)
                {
                    directionMultiplierX = 1;
                }
                if (shots.at(i).lastYMoveDirection == 0)
                {
                    directionMultiplierY = 1;
                }
                D2D1_RECT_F destRect = D2D1::RectF(screenCenterX, screenCenterY,
                    (shots.at(i).scale * size.width) + screenCenterX, (shots.at(i).scale * size.height) + screenCenterY);

                // Calculate the center of the destRect for rotation
                D2D1_POINT_2F center = D2D1::Point2F(
                    (destRect.left + destRect.right) / 2.0f,
                    (destRect.top + destRect.bottom) / 2.0f
                );

                // Calculate the rotation angle based on player's movement direction
                double angle = atan2(static_cast<double>(shots.at(i).yDirection), static_cast<double>(shots.at(i).xDirection));

                // Convert radians to degrees if necessary (for debugging or if the Rotation function expects degrees)
                double angleDegrees = 90 + (angle * (180.0 / 3.14));

                // Create a rotation transform
                D2D1_MATRIX_3X2_F rotation = D2D1::Matrix3x2F::Rotation(angleDegrees, center); // If Rotation function expects degrees

                // Set the transform on the render target
                pRenderTarget->SetTransform(&rotation);

                // Draw the bitmap with the rotation applied
                pRenderTarget->DrawBitmap(shotBitmap, destRect, 1.0F, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);

                // Reset the transform to identity so other draw calls aren't affected
                pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
            }
        }

        HRESULT hr = S_OK;

        // Create DirectWrite factory and text format for rendering FPS counter
        hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(&pDWriteFactory)
        );
        if (SUCCEEDED(hr)) {
            hr = pDWriteFactory->CreateTextFormat(
                L"Arial",
                NULL,
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                20.0f,
                L"en-us",
                &pTextFormat
            );
        }

        // Create the brush for text
        if (SUCCEEDED(hr)) {
            hr = pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::White),
                &pBrush
            );
        }

        hr = pRenderTarget->EndDraw();
        if (hr == D2DERR_RECREATE_TARGET)
        {
            DiscardDeviceResources();
        }
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

Object background;
std::vector<Object> spriteData;
Player player;
Enemy enemy[1];
std::vector<Enemy> enemies;
Shot shot[1000];
std::vector<Shot> shots;
int shotsUsed = 0;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    /* initialize random seed: */
    srand(time(NULL));

    

    // Load all Sprites
    LoadSpriteData(spriteData);

    enemies.emplace_back(enemy[0]);

    lastUpdate = std::chrono::steady_clock::now();
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
    if (SUCCEEDED(hr))
    {

        WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hInstance;
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = NULL;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = L"Direct2DPNGWindow";
        wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

        if (RegisterClassEx(&wc))
        {
            HWND hWnd = CreateWindowEx(0, L"Direct2DPNGWindow", L"Direct2D PNG Example", WS_POPUP | WS_VISIBLE, 0, 0,
                sysScreenX, sysScreenY, NULL, NULL, hInstance, NULL);

            if (hWnd)
            {
                ShowWindow(hWnd, nCmdShow);

                MSG msg;
                while (GetMessage(&msg, NULL, 0, 0))
                {
                    if (keys.escape == true)
                    {
                        DestroyWindow(hWnd);
                    }
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }

        pD2DFactory->Release();
    }

    return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_KEYDOWN:
    {
        switch (wParam)
        {
        case VK_LEFT:
            keys.left = true;
            break;
        case VK_RIGHT:
            keys.right = true;
            break;
        case VK_UP:
            keys.up = true;
            break;
        case VK_DOWN:
            keys.down = true;
            break;
        case VK_SPACE:
            keys.space = true;
            break;
        case VK_SHIFT:
            keys.lShift = true;
            break;
        case VK_ESCAPE:
            keys.escape = true;
            break;
        }
        InvalidateRect(hWnd, NULL, true);
        break;
    }
    case WM_KEYUP:
    {
        switch (wParam)
        {
        case VK_LEFT:
            keys.left = false;
            break;
        case VK_RIGHT:
            keys.right = false;
            break;
        case VK_UP:
            keys.up = false;
            break;
        case VK_DOWN:
            keys.down = false;
            break;
        case VK_SPACE:
            keys.space = false;
            break;
        case VK_SHIFT:
            keys.lShift = false;
            break;
        case VK_ESCAPE:
            keys.escape = false;
            break;
        }
        InvalidateRect(hWnd, NULL, true);
        break;
    }

    case WM_PAINT:
    case WM_DISPLAYCHANGE:
    {
        std::chrono::steady_clock::time_point currentFrameTime = std::chrono::steady_clock::now();
        std::chrono::duration<float> elapsedTime = currentFrameTime - lastMoveTime;

        if (keys.escape == true)
        {
            break;
        }

        for (int i = 0; i < enemies.size(); i++)
        {
            if (enemies.at(i).framesWalked2 % 288 == 0 && (std::chrono::steady_clock::now() - enemies.at(i).hitStunStartTime) >= enemies.at(i).hitStunTime)
            {
                double xDirection, yDirection, angle;

                angle = atan2(player.yPosition - enemies.at(i).yPosition, player.xPosition - enemies.at(i).xPosition);


                double x(cos(angle)), y(sin(angle));


                ApplyEnemyDirectionalInput(enemies.at(i), 2 * cos(angle), 2 * sin(angle));
            }
            else if ((std::chrono::steady_clock::now() - enemies.at(i).hitStunStartTime) >= enemies.at(i).hitStunTime)
            {
                double xDirection, yDirection, angle;

                angle = atan2(player.yPosition - enemies.at(i).yPosition, player.xPosition - enemies.at(i).xPosition);

                ApplyEnemyDirectionalInput(enemies.at(i), cos(angle), sin(angle));
            }
        }

        double xDir, yDir;

        /*if (keys.space == true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }*/
            GetDirectionalInput(xDir, yDir, keys.right, keys.left, keys.down, keys.up);
            if (keys.lShift == true)
            {
                xDir *= 2;
                yDir *= 2;
                player.moveAnimationInterval = std::chrono::nanoseconds(41666666);
            }
            else
            {
                player.moveAnimationInterval = std::chrono::nanoseconds(166666666);
            }
            ApplyPlayerDirectionalInput(player, elapsedTime, currentFrameTime, .5 * xDir, 0.5 * yDir);

        player.timeSinceLastShot = std::chrono::steady_clock::now() - player.timeOfLastShot;
        if (player.shotCooldown == false && player.timeSinceLastShot >= std::chrono::nanoseconds(250000000))
        {
            player.shotCooldown = true;
        }

        if (keys.space == true && player.shotCooldown == true)
        {
            player.BasicShot(enemies);
            shots.emplace_back(shot[shotsUsed]);
            shotsUsed++;
            
            shots.back().InitializeShot(player);
        }
        for (int i = 0; i < shots.size(); i++)
        {
            shots.at(i).DisplaceShot();
        }

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        OnRender(hWnd, spriteData, player, enemies, shots);

        // Increment frame count for FPS computation
        frameCount++;

        EndPaint(hWnd, &ps);

        // Invalidate the window rectangle to cause WM_PAINT message to be generated
        InvalidateRect(hWnd, NULL, FALSE);
    } break;

    case WM_DESTROY: {
        DiscardDeviceResources();
        PostQuitMessage(0);
    } break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}