# lua_webp

lua_webp is a Lua module for performing WebP image format conversion in Lua. It provides cwebp and dwebp functions for converting images to WebP format and converting WebP images to other formats.

## Installation

Import the lua_webp module into your Lua project. You can obtain the latest version of the module file from the project's GitHub repository.

## Usage

Here's an example of using the lua_webp module for image format conversion:

```lua
-- Import the lua_webp module
local webp = require "lua_webp"

-- Get the cwebp and dwebp functions
local cwebp = webp.cwebp
local dwebp = webp.dwebp

-- Convert images to WebP format

-- Use the cwebp function to write the output of the path2Webp function to the test.webp file
local outWebp = cwebp:path2Webp("test.jpg", {
    quality = 10
})
local file = io.open("test.webp", "w")
file:write(outWebp)

-- Read the contents of the test.jpg file
local file2 = io.open("test.jpg", "r")
local jpgData = file2:read("*a") 

-- Use the cwebp function to convert the image data to WebP format and write the result to the test2.webp file
local outWebp2 = cwebp:image2Webp(jpgData, { quality = 10 })
local file3 = io.open("test2.webp", "w")
file3:write(outWebp2)

-- Convert WebP images to other formats

-- Use the dwebp function to convert the test.webp file to the PNG format and write the result to the webp2png1.png file
local r = dwebp:path2Image("test.webp", "png", {
    use_threads = 1
})
local file4 = io.open("webp2png1.png", "w")
file4:write(r)

-- Use the dwebp function to convert the WebP image data in the outWebp2 variable to the PNG format and write the result to the webp2png2.png file
local r = dwebp:webp2Image(outWebp2, "png", {})
local file5 = io.open("webp2png2.png", "w")
file5:write(r)
```

Note that you can customize the conversion parameters such as quality settings according to your needs.

## Contributing

Contributions to this project are welcome! If you encounter any issues or have suggestions for improvements, please submit issues or pull requests.

## License

This project is distributed under the MIT License. For more information, see the [LICENSE](LICENSE) file.
