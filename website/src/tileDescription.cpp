// Enums defined in ImfTileDescription.h
// enum LevelMode
// {
//     ONE_LEVEL,
//     MIPMAP_LEVELS,
//     RIPMAP_LEVELS
// };
    
// enum LevelRoundingMode
// {
//     ROUND_DOWN,
//     ROUND_UP
// };
    
class TileDescription
{
  public:
    unsigned int xSize; // size of a tile in the x dimension
    unsigned int ySize; // size of a tile in the y dimension
    LevelMode mode;
    LevelRoundingMode roundingMode;
    // ...  (methods omitted)
};