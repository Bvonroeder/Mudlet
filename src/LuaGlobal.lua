
----------------------------------------------------------------------------------
-- Useful global LUA functions that are accessible from within Mudlet
----------------------------------------------------------------------------------
-- These general functions can be used from anywhere within Mudlet scripts
-- They are described in the manual.
----------------------------------------------------------------------------------



----------------------------------------------------------------------------------
-- Functions written by Blaine von Roeder - December 2009
----------------------------------------------------------------------------------

atcp = {}

--Overloadable function for ATCP Events
function handleAtcpEvent(eventName, eventArgs)
-- EventName is "vitals","exits","roomname"
-- EventArgs is the new info(ex: exits has args "u,d")
end


-- This function flags a variable to be saved by Mudlet's variable persistence system.
-- Usage: remember("varName")
-- Example: remember("table_Weapons")
-- Example: remember("var_EnemyHeight")
-- Variables are automatically unpacked into the global namespace when the profile is loaded. 
-- They are saved to "SavedVariables.lua" when the profile is closed or saved.

function remember(varName)
	if not _saveTable then
		_saveTable = {}
	end

    _saveTable[varName] = _G[varName]
end


--- This function should be primarily used by Mudlet. It loads saved settings in from the Mudlet home directory
--- and unpacks them into the global namespace. 
function loadVars()
	if string.char(getMudletHomeDir():byte()) == "/" then _sep = "/" else  _sep = "\\" end
	local l_SettingsFile = getMudletHomeDir() .. _sep .. "SavedVariables.lua"
	local lt_VariableHolder = {}
	if (io.exists(l_SettingsFile)) then
		table.load(l_SettingsFile, lt_VariableHolder)
		for k,v in pairs(lt_VariableHolder) do
				_G[k] = v
		end
	end
end

-- This function should primarily be used by Mudlet. It saves the contents of _saveTable into a file for persistence.

function saveVars()
	if string.char(getMudletHomeDir():byte()) == "/" then _sep = "/" else  _sep = "\\" end
	local l_SettingsFile = getMudletHomeDir() .. _sep .. "SavedVariables.lua"
    for k,_ in pairs(_saveTable) do
        remember(k)
    end
	table.save(l_SettingsFile, _saveTable)
end

-- Move a custom gauge built by createGauge(...)
-- Example: moveGauge("healthBar", 1200, 400)
-- This would move the health bar gauge to the location 1200, 400

function moveGauge(gaugeName, newX, newY)
	for _,g in pairs(gaugesTable) do
		if g.name == gaugeName then
			moveWindow(gaugeName, newX, newY)
			moveWindow(gaugeName .. "_back", newX, newY)
		end
	end
end

-- Set the text on a custom gauge built by createGauge(...)
-- Example: setGaugeText("healthBar", "HP: 100%", 40, 40, 40)
-- Example: setGaugeText("healthBar", "HP: 100%", "red")
-- An empty gaugeText will clear the text entirely.
-- Colors are optional and will default to 0,0,0(black) if not passed as args.

function setGaugeText(gaugeName, gaugeText, color1, color2, color3)
	local red,green,blue = 0,0,0
	local l_labelText = gaugeText
	
	if color1 ~= nil then
		if color2 == nil then
			red, green, blue = getRGB(color1)
		else
			red, green, blue = color1, color2, color3
		end
	end
	
	-- Check to make sure we had a text to apply, if not, clear the text
	if l_labelText == nil then
		l_labelText = ""
	end
	
	local l_EchoString = [[<font color="#]] .. RGB2Hex(red,green,blue) .. [[">]] .. l_labelText .. [[</font>]]
	

	echo(gaugeName, l_EchoString)
	echo(gaugeName .. "_back", l_EchoString)
end

-- Converts an RGB value into an HTML compliant(label usable) HEX number
-- This function is colorNames aware and can take any defined global color as its first arg
-- Example: RGB2Hex(255,255,255) returns "FFFFFF"
-- Example: RGB2Hex("white") returns "FFFFFF"
function RGB2Hex(red,green,blue)
	local l_Red, l_Green, l_Blue = 0,0,0
	if green == nil then -- Not an RGB but a "color" instead!
		l_Red, l_Green, l_Blue = getRGB(red)
	else -- Nope, true color here
		l_Red, l_Green, l_Blue = red, green, blue
	end
									
	return PadHexNum(string.format("%X",l_Red)) .. 
			PadHexNum(string.format("%X",l_Green)) .. 
			PadHexNum(string.format("%X",l_Blue))
end

-- Pads a hex number to ensure a minimum of 2 digits.
-- Example: PadHexNum("F") returns "F0
function PadHexNum(incString)
	local l_Return = incString
	if tonumber(incString,16)<16 then
		if tonumber(incString,16)<10 then
			l_Return = "0" .. l_Return
		elseif tonumber(incString,16)>10 then
			l_Return = l_Return .. "0"
		end
	end

	return l_Return
end

-----------------------------------------------------------
-- Functions written by John Dahlstrom November 2008
-----------------------------------------------------------
  
-- Example: 
--
-- local red, green, blue = getRGB("green")
-- echo(red .. "." .. green .. "." .. blue ) 
--
-- This would then display 0.255.0 on your screen.

function getRGB(colorName)

	local red = color_table[colorName][1]
	local green = color_table[colorName][2]
	local blue = color_table[colorName][3]

	return red, green, blue
	
end


-- Make your very own customized gauge with this function.
--
-- Example: 
--
-- createGauge("healthBar", 300, 20, 30, 300, nil, 0, 255, 0) 
-- or 
-- createGauge("healthBar", 300, 20, 30, 300, nil, "green")
--
-- This would make a gauge at that's 300px width, 20px in height, located at Xpos and Ypos and is green.
-- The second example is using the same names you'd use for something like fg() or bg().
--
-- If you wish to have some text on your label, you'll change the nil part and make it look like this:
-- createGauge("healthBar", 300, 20, 30, 300, "Now with some text", 0, 255, 0) 
-- or 
-- createGauge("healthBar", 300, 20, 30, 300, "Now with some text", "green")

gaugesTable = {} -- first we need to make this table which will be used later to store important data in...

function createGauge(gaugeName, width, height, Xpos, Ypos, gaugeText, color1, color2, color3)
	-- Check for existing gauge with same name
	-- By Blaine von Roeder - 12/2009
	for _,t in pairs(gaugesTable) do
		if (t.name == gaugeName) then
			return
		end
	end

	-- make a nice background for our gauge
	createLabel(gaugeName .. "_back",0,0,0,0,1)
	if color2 == nil then
		local red, green, blue = getRGB(color1)
		setBackgroundColor(gaugeName .. "_back", red , green, blue, 100)		
	else
		setBackgroundColor(gaugeName .. "_back", color1 ,color2, color3, 100)
	end
	moveWindow(gaugeName .. "_back", Xpos, Ypos)
	resizeWindow(gaugeName .. "_back", width, height)
	showWindow(gaugeName .. "_back")

	-- make a nicer front for our gauge
	createLabel(gaugeName,0,0,0,0,1)
	if color2 == nil then
		local red, green, blue = getRGB(color1)
		setBackgroundColor(gaugeName, red , green, blue, 255)		
	else
		setBackgroundColor(gaugeName, color1 ,color2, color3, 255)
	end
	moveWindow(gaugeName, Xpos, Ypos)
	resizeWindow(gaugeName, width, height)
	showWindow(gaugeName)

	-- Set Gauge text (Defaults to black)
	-- If no gaugeText was passed, we'll just leave it blank!
	if gaugeText ~= nil then
		setGaugeText(gaugeName, gaugeText, "black")
	else
		setGaugeText(gaugeName)
	end

	-- store important data in a table
	table.insert(gaugesTable, {name = gaugeName, width = width, height = height, color1 = color1, color2 = color2, color3 = color3})
	
end


-- Use this function when you want to change the gauges look according to your values.
--
-- Example: 
--
-- setGauge("healthBar", 200, 400)
--
-- In that example, we'd change the looks of the gauge named healthBar and make it fill
-- to half of its capacity. The height is always remembered.
--
-- If you wish to change the text on your gauge, you'd do the following:
--
-- setGauge("healthBar", 200, 400, "some text")
--
-- Typical usage would be in a prompt with your current health or whatever value, and throw
-- in some variables instead of the numbers.

function setGauge(gaugeName, currentValue, maxValue, gaugeText)	


	-- search through our gaugesTable for our name input and change according to the values
	for _,v in pairs(gaugesTable) do
		if v.name == gaugeName then
			resizeWindow(gaugeName, v.width/100*((100/maxValue)*currentValue), v.height)
		end
	end

	-- if we wanted to change the text, we do it
	if gaugeText ~= nil then
		echo(gaugeName .. "_back", gaugeText)
		echo(gaugeName, gaugeText)
	end
	
end


-- Make a new console window with ease. The default background is black and text color white.
--
-- Example:
-- -- createConsole("myConsoleWindow", 8, 80, 20, 200, 400)
-- -- This will create a miniconsole window that has a font size of 8pt, will display 80 characters in width, -- hold a maximum of 20 lines and be place at 200x400 of your mudlet window. -- If you wish to change the color you can easily do this when updating your text or manually somewhere, using -- setFgColor() and setBackgroundColor(). function createConsole(consoleName, fontSize, charsPerLine, numberOfLines, Xpos, Ypos) createMiniConsole(consoleName,0,0,1,1) setMiniConsoleFontSize(consoleName, fontSize) local x,y = calcFontSize( fontSize ) resizeWindow(consoleName, x*charsPerLine, y*numberOfLines) setWindowWrap(consoleName, Xpos) moveWindow(consoleName, Xpos, Ypos) setBackgroundColor(consoleName,0,0,0,0) setFgColor(consoleName, 255,255,255) end 

-- Send any amount of commands to the MUD
-- Example: sendAll("smile", "dance", "laugh")
function sendAll( what, ... )
	if table.maxn(arg) == 0 then
  		send( what )
    else
    	send(what)
    	for i,v in ipairs(arg) do
	    	-- v = the value of the arg
	    	send(v)
	    end
    end
end

         
-- Echo something after your line
function suffix(what)
	local length = string.len(line)
	moveCursor("main", length-1, getLineNumber())
	insertText(what)
end


-- Echo something before your line
function prefix(what)
    moveCursor("main",0,getLineNumber());
	insertText(what)
end


-- Gag the whole line
function gagLine()
	--selectString(line, 1)
	--replace("")
 deleteLine()
end


-- Replace all words on the current line by your choice

-- Example: replaceAll("John", "Doe")
-- This will replace the word John with the word Doe, everytime the word John occurs on the current line.
function replaceAll(word, what)
	while selectString(word, 1) > 0 do replace(what) end
end


-- Replace the whole with a string you'd like.
function replaceLine(what)
	selectString(line, 1)
	replace("")
	insertText(what)
end


-----------------------------------
-- some functions from Heiko
----------------------------------

-- default resizeEvent handler function.
-- overwrite this function to make a custom event handler if the main window is being resized
function handleResizeEvent()
end

function deselect()
	selectString("",1);
end

-- Function shows the content of a Lua table on the screen
function printTable( map )
	echo("-------------------------------------------------------\n");
	for k, v in pairs( map ) do
		echo( "key=" .. k .. " value=" .. v .. "\n" )
	end
	echo("-------------------------------------------------------\n");
end

function __printTable( k, v )
  insertText ("\nkey = " .. tostring (k) .. " value = " .. tostring( v )  )
end 

-- Function colorizes all matched regex capture groups on the screen 
function showCaptureGroups()
    for k, v in pairs ( matches ) do
        selectCaptureGroup( tonumber(k) )
        setFgColor( math.random(0,255), math.random(0,255), math.random(0,255) )
        setBgColor( math.random(0,255), math.random(0,255), math.random(0,255) )
    end
end

-- prints the content of the table multimatches[n][m] to the screen
-- this is meant as a tool to help write multiline trigger scripts
-- This helps you to easily see what your multiline trigger actually captured in all regex
-- You can use these values directly in your script by referring to it with multimatches[regex-number][capturegroup]
function showMultimatches()
    echo("\n-------------------------------------------------------");
    echo("\nThe table multimatches[n][m] contains:");
    echo("\n-------------------------------------------------------");
    for k,v in ipairs(multimatches) do
       	echo("\nregex " .. k .. " captured: (multimatches["..k .."][1-n])");
       	for k2,v2 in ipairs(v) do
           	echo("\n          key="..k2.." value="..v2);
       	end
    end
    echo("\n-------------------------------------------------------\n");
end

function listPrint( map )
	echo("-------------------------------------------------------\n");
	for k,v in ipairs( map ) do
		echo( k .. ". ) "..v .. "\n" );
	end
	echo("-------------------------------------------------------\n");
end

function listAdd( list, what )
	table.insert( list, what );
end


function listRemove( list, what )
	for k,v in ipairs( list ) do
		if v == what then
			table.remove( list, k )
		end
	end
end

-------------------------------------------------------------------
--.... some functions from Tichi 2009
-------------------------------------------------------------------
-- Gets the actual size of a non-numerical table
function table.size(t)
        if not t then
                return 0
        end

        local i = 0
        for k, v in pairs(t) do
                i = i + 1
        end
        return i
end

-- Checks to see if a file exists
function io.exists(file)
        local f = io.open(file)
        if f then
                io.close(f)
                return true
        end
        return false
end

-- Splits a string
function string:split(delimiter)
        local result = { }
        local from  = 1
        local delim_from, delim_to = string.find( self, delimiter, from  )
        while delim_from do
                table.insert( result, string.sub( self, from , delim_from-1 ) )
                from  = delim_to + 1
                delim_from, delim_to = string.find( self, delimiter, from  )
        end
        table.insert( result, string.sub( self, from  ) )
        return result
end

-- Determines if a table contains a value as a key or as a value (recursive)
function table.contains(t, value)
        for k, v in pairs(t) do
                if v == value then
                        return true
                elseif k == value then
                        return true
                elseif type(v) == "table" then
                        if table.contains(v, value) then return true end
                end
        end

        return false
end



-----------------------------------------------------------
-- some functions from Vadim Peretrokin 2009
-----------------------------------------------------------

-------------------------------------------------------------------------------
--                     Color Definitions
--------------------------------------------------------------------------------
-- These color definitions are intended to be used in conjunction with fg() 
-- and bg() colorizer functions that are defined further below
--------------------------------------------------------------------------------        
        
color_table = {
        snow                  = {255, 250, 250},
        ghost_white           = {248, 248, 255},
        GhostWhite            = {248, 248, 255},
        white_smoke           = {245, 245, 245},
        WhiteSmoke            = {245, 245, 245},
        gainsboro             = {220, 220, 220},
        floral_white          = {255, 250, 240},
        FloralWhite           = {255, 250, 240},
        old_lace              = {253, 245, 230},
        OldLace               = {253, 245, 230},
        linen                 = {250, 240, 230},
        antique_white         = {250, 235, 215},
        AntiqueWhite          = {250, 235, 215},
        papaya_whip           = {255, 239, 213},
        PapayaWhip            = {255, 239, 213},
        blanched_almond       = {255, 235, 205},
        BlanchedAlmond        = {255, 235, 205},
        bisque                = {255, 228, 196},
        peach_puff            = {255, 218, 185},
        PeachPuff             = {255, 218, 185},
        navajo_white          = {255, 222, 173},
        NavajoWhite           = {255, 222, 173},
        moccasin              = {255, 228, 181},
        cornsilk              = {255, 248, 220},
        ivory                 = {255, 255, 240},
        lemon_chiffon         = {255, 250, 205},
        LemonChiffon          = {255, 250, 205},
        seashell              = {255, 245, 238},
        honeydew              = {240, 255, 240},
        mint_cream            = {245, 255, 250},
        MintCream             = {245, 255, 250},
        azure                 = {240, 255, 255},
        alice_blue            = {240, 248, 255},
        AliceBlue             = {240, 248, 255},
        lavender              = {230, 230, 250},
        lavender_blush        = {255, 240, 245},
        LavenderBlush         = {255, 240, 245},
        misty_rose            = {255, 228, 225},
        MistyRose             = {255, 228, 225},
        white                 = {255, 255, 255},
        black                 = {0, 0, 0},
        dark_slate_gray       = {47, 79, 79},
        DarkSlateGray         = {47, 79, 79},
        dark_slate_grey       = {47, 79, 79},
        DarkSlateGrey         = {47, 79, 79},
        dim_gray              = {105, 105, 105},
        DimGray               = {105, 105, 105},
        dim_grey              = {105, 105, 105},
        DimGrey               = {105, 105, 105},
        slate_gray            = {112, 128, 144},
        SlateGray             = {112, 128, 144},
        slate_grey            = {112, 128, 144},
        SlateGrey             = {112, 128, 144},
        light_slate_gray      = {119, 136, 153},
        LightSlateGray        = {119, 136, 153},
        light_slate_grey      = {119, 136, 153},
        LightSlateGrey        = {119, 136, 153},
        gray                  = {190, 190, 190},
        grey                  = {190, 190, 190},
        light_grey            = {211, 211, 211},
        LightGrey             = {211, 211, 211},
        light_gray            = {211, 211, 211},
        LightGray             = {211, 211, 211},
        midnight_blue         = {25, 25, 112},
        MidnightBlue          = {25, 25, 112},
        navy                  = {0, 0, 128},
        navy_blue             = {0, 0, 128},
        NavyBlue              = {0, 0, 128},
        cornflower_blue       = {100, 149, 237},
        CornflowerBlue        = {100, 149, 237},
        dark_slate_blue       = {72, 61, 139},
        DarkSlateBlue         = {72, 61, 139},
        slate_blue            = {106, 90, 205},
        SlateBlue             = {106, 90, 205},
        medium_slate_blue     = {123, 104, 238},
        MediumSlateBlue       = {123, 104, 238},
        light_slate_blue      = {132, 112, 255},
        LightSlateBlue        = {132, 112, 255},
        medium_blue           = {0, 0, 205},
        MediumBlue            = {0, 0, 205},
        royal_blue            = {65, 105, 225},
        RoyalBlue             = {65, 105, 225},
        blue                  = {0, 0, 255},
        dodger_blue           = {30, 144, 255},
        DodgerBlue            = {30, 144, 255},
        deep_sky_blue         = {0, 191, 255},
        DeepSkyBlue           = {0, 191, 255},
        sky_blue              = {135, 206, 235},
        SkyBlue               = {135, 206, 235},
        light_sky_blue        = {135, 206, 250},
        LightSkyBlue          = {135, 206, 250},
        steel_blue            = {70, 130, 180},
        SteelBlue             = {70, 130, 180},
        light_steel_blue      = {176, 196, 222},
        LightSteelBlue        = {176, 196, 222},
        light_blue            = {173, 216, 230},
        LightBlue             = {173, 216, 230},
        powder_blue           = {176, 224, 230},
        PowderBlue            = {176, 224, 230},
        pale_turquoise        = {175, 238, 238},
        PaleTurquoise         = {175, 238, 238},
        dark_turquoise        = {0, 206, 209},
        DarkTurquoise         = {0, 206, 209},
        medium_turquoise      = {72, 209, 204},
        MediumTurquoise       = {72, 209, 204},
        turquoise             = {64, 224, 208},
        cyan                  = {0, 255, 255},
        light_cyan            = {224, 255, 255},
        LightCyan             = {224, 255, 255},
        cadet_blue            = {95, 158, 160},
        CadetBlue             = {95, 158, 160},
        medium_aquamarine     = {102, 205, 170},
        MediumAquamarine      = {102, 205, 170},
        aquamarine            = {127, 255, 212},
        dark_green            = {0, 100, 0},
        DarkGreen             = {0, 100, 0},
        dark_olive_green      = {85, 107, 47},
        DarkOliveGreen        = {85, 107, 47},
        dark_sea_green        = {143, 188, 143},
        DarkSeaGreen          = {143, 188, 143},
        sea_green             = {46, 139, 87},
        SeaGreen              = {46, 139, 87},
        medium_sea_green      = {60, 179, 113},
        MediumSeaGreen        = {60, 179, 113},
        light_sea_green       = {32, 178, 170},
        LightSeaGreen         = {32, 178, 170},
        pale_green            = {152, 251, 152},
        PaleGreen             = {152, 251, 152},
        spring_green          = {0, 255, 127},
        SpringGreen           = {0, 255, 127},
        lawn_green            = {124, 252, 0},
        LawnGreen             = {124, 252, 0},
        green                 = {0, 255, 0},
        chartreuse            = {127, 255, 0},
        medium_spring_green   = {0, 250, 154},
        MediumSpringGreen     = {0, 250, 154},
        green_yellow          = {173, 255, 47},
        GreenYellow           = {173, 255, 47},
        lime_green            = {50, 205, 50},
        LimeGreen             = {50, 205, 50},
        yellow_green          = {154, 205, 50},
        YellowGreen           = {154, 205, 50},
        forest_green          = {34, 139, 34},
        ForestGreen           = {34, 139, 34},
        olive_drab            = {107, 142, 35},
        OliveDrab             = {107, 142, 35},
        dark_khaki            = {189, 183, 107},
        DarkKhaki             = {189, 183, 107},
        khaki                 = {240, 230, 140},
        pale_goldenrod        = {238, 232, 170},
        PaleGoldenrod         = {238, 232, 170},
        light_goldenrod_yellow= {250, 250, 210},
        LightGoldenrodYellow  = {250, 250, 210},
        light_yellow          = {255, 255, 224},
        LightYellow           = {255, 255, 224},
        yellow                = {255, 255, 0},
        gold                  = {255, 215, 0},
        light_goldenrod       = {238, 221, 130},
        LightGoldenrod        = {238, 221, 130},
        goldenrod             = {218, 165, 32},
        dark_goldenrod        = {184, 134, 11},
        DarkGoldenrod         = {184, 134, 11},
        rosy_brown            = {188, 143, 143},
        RosyBrown             = {188, 143, 143},
        indian_red            = {205, 92, 92},
        IndianRed             = {205, 92, 92},
        saddle_brown          = {139, 69, 19},
        SaddleBrown           = {139, 69, 19},
        sienna                = {160, 82, 45},
        peru                  = {205, 133, 63},
        burlywood             = {222, 184, 135},
        beige                 = {245, 245, 220},
        wheat                 = {245, 222, 179},
        sandy_brown           = {244, 164, 96},
        SandyBrown            = {244, 164, 96},
        tan                   = {210, 180, 140},
        chocolate             = {210, 105, 30},
        firebrick             = {178, 34, 34},
        brown                 = {165, 42, 42},
        dark_salmon           = {233, 150, 122},
        DarkSalmon            = {233, 150, 122},
        salmon                = {250, 128, 114},
        light_salmon          = {255, 160, 122},
        LightSalmon           = {255, 160, 122},
        orange                = {255, 165, 0},
        dark_orange           = {255, 140, 0},
        DarkOrange            = {255, 140, 0},
        coral                 = {255, 127, 80},
        light_coral           = {240, 128, 128},
        LightCoral            = {240, 128, 128},
        tomato                = {255, 99, 71},
        orange_red            = {255, 69, 0},
        OrangeRed             = {255, 69, 0},
        red                   = {255, 0, 0},
        hot_pink              = {255, 105, 180},
        HotPink               = {255, 105, 180},
        deep_pink             = {255, 20, 147},
        DeepPink              = {255, 20, 147},
        pink                  = {255, 192, 203},
        light_pink            = {255, 182, 193},
        LightPink             = {255, 182, 193},
        pale_violet_red       = {219, 112, 147},
        PaleVioletRed         = {219, 112, 147},
        maroon                = {176, 48, 96},
        medium_violet_red     = {199, 21, 133},
        MediumVioletRed       = {199, 21, 133},
        violet_red            = {208, 32, 144},
        VioletRed             = {208, 32, 144},
        magenta               = {255, 0, 255},
        violet                = {238, 130, 238},
        plum                  = {221, 160, 221},
        orchid                = {218, 112, 214},
        medium_orchid         = {186, 85, 211},
        MediumOrchid          = {186, 85, 211},
        dark_orchid           = {153, 50, 204},
        DarkOrchid            = {153, 50, 204},
        dark_violet           = {148, 0, 211},
        DarkViolet            = {148, 0, 211},
        blue_violet           = {138, 43, 226},
        BlueViolet            = {138, 43, 226},
        purple                = {160, 32, 240},
        medium_purple         = {147, 112, 219},
        MediumPurple          = {147, 112, 219},
        thistle               = {216, 191, 216}
}


-----------------------------------------------------------------------------------
--            Color Functions
-------------------------------------------------------------------------------------
-- sets current background color to a named color see table above
-- usage: bg( "magenta" )

function bg(name)
    setBgColor(color_table[name][1], color_table[name][2], color_table[name][3])
end

-- sets current foreground color see bg( ) above
function fg(name)
    setFgColor(color_table[name][1], color_table[name][2], color_table[name][3])
end

---------------------------------------------------------------------------------------
--              Save & Load Variables
---------------------------------------------------------------------------------------
-- The below functions can be used to save individual Lua tables to disc and load
-- them again at a later time e.g. make a database, collect statistical information etc.
-- These functions are also used by Mudlet to load & save the entire Lua session variables
--
-- table.load(file)   - loads a serialized file into the globals table (only Mudlet should use this)
-- table.load(file, table) - loads a serialized file into the given table
 
-- table.save(file)  - saves the globals table (minus some lua enviroment stuffs) into a file (only Mudlet should use this)
-- table.save(file, table) - saves the given table into the given file
-- 
-- Original code written by CHILLCODE™ on https://board.ptokax.ch, distributed under the same terms as Lua itself.
-- 
-- Notes:
--  Userdata and indices of these are not saved
--  Functions are saved via string.dump, so make sure it has no upvalues
--  References are saved
--

function table.save( sfile, t )
 if t == nil then t = _G end
 local tables = {}
 table.insert( tables, t )
 local lookup = { [t] = 1 }
 local file = io.open( sfile, "w" )
 file:write( "return {" )
 for i,v in ipairs( tables ) do
  table.pickle( v, file, tables, lookup )
 end
 file:write( "}" )
 file:close()
end
 
function table.pickle( t, file, tables, lookup )
 file:write( "{" )
 for i,v in pairs( t ) do
  -- escape functions
  if type( v ) ~= "function" and type( v ) ~= "userdata" and (i ~= "string" and i ~= "xpcall" and i ~= "package" and i ~= "os" and i ~= "io" and i ~= "math" and i ~= "debug" and i ~= "coroutine" and i ~= "_G" and i ~= "_VERSION" and i ~= "table") then
   -- handle index
   if type( i ) == "table" then
    if not lookup[i] then
     table.insert( tables, i )
     lookup[i] = table.maxn( tables )
    end
    file:write( "[{"..lookup[i].."}] = " )
   else
    local index = ( type( i ) == "string" and "[ "..string.enclose( i, 50 ).." ]" ) or string.format( "[%d]", i )
    file:write( index.." = " )
   end
   -- handle value
   if type( v ) == "table" then
    if not lookup[v] then
     table.insert( tables, v )
     lookup[v] = table.maxn( tables )
    end
    file:write( "{"..lookup[v].."}," )
   else
    local value =  ( type( v ) == "string" and string.enclose( v, 50 ) ) or tostring( v )
    file:write( value.."," )
   end
  end
 end
 file:write( "},\n" )  
end
 
-- enclose string by long brakets ( string, maxlevel )
function string.enclose( s, maxlevel )
 s = "["..s.."]"
 local level = 0
 while 1 do
  if maxlevel and level == maxlevel then
   error( "error: maxlevel too low, "..maxlevel )
  -- 
  elseif string.find( s, "%["..string.rep( "=", level ).."%[" ) or string.find( s, "]"..string.rep( "=", level ).."]" ) then
   level = level + 1
  else
   return "["..string.rep( "=", level )..s..string.rep( "=", level ).."]"
  end
 end
end

function table.load( sfile, loadinto )
 local tables = dofile( sfile )
 if tables then
  if loadinto ~= nil and type(loadinto) == "table" then
   table.unpickle( tables[1], tables, loadinto )
  else
   table.unpickle( tables[1], tables, _G )
  end
 end
end
 
function table.unpickle( t, tables, tcopy, pickled )
 pickled = pickled or {}
 pickled[t] = tcopy
 for i,v in pairs( t ) do
  local i2 = i
  if type( i ) == "table" then
   local pointer = tables[ i[1] ]
   if pickled[pointer] then
    i2 = pickled[pointer]
   else
    i2 = {}
    table.unpickle( pointer, tables, i2, pickled )
   end
  end
  local v2 = v
  if type( v ) == "table" then
   local pointer = tables[ v[1] ]
   if pickled[pointer] then
    v2 = pickled[pointer]
   else
    v2 = {}
    table.unpickle( pointer, tables, v2, pickled )
   end
  end
  tcopy[i2] = v2
 end
end

-- Replaces the given wildcard (as a number) with the given text.
--
-- -- Example: replaceWildcard(1, "hello") on a trigger of `^You wave (goodbye)\.$`
function replaceWildcard(what, replacement)
    if replacement == nil or what == nil then 
        return 
    end
    selectCaptureGroup(what)
    replace(replacement)
end

----------------------------------------------------------------------------------
-- function by Ryan: pretty print function for tables
----------------------------------------------------------------------------------
-- usage: display( mytable )
----------------------------------------
-- pretty display function
function display(what, numformat, recursion)
  recursion = recursion or 0

  if recursion == 0 then
    echo("\n")
--    echo("-------------------------------------------------------\n")
  end
  echo(printable(what, numformat))

  -- Do all the stuff inside a table
  if type(what) == 'table' then
    echo(" {")

    local firstline = true   -- a kludge so empty tables print on one line
    for k, v in pairs(what) do
      if firstline then echo("\n"); firstline = false end
      echo(indent(recursion))
      echo(printable(k))
      echo(": ")
      display(v, numformat, recursion + 1)
    end

    -- so empty tables print as {} instead of {..indent..}
    if not firstline then echo(indent(recursion - 1)) end
    echo("}")
  end

  echo("\n")
  if recursion == 0 then
--    echo ("-------------------------------------------------------\n")
  end
end

-- Basically like tostring(), except takes a numformat
-- and is a little better suited for working with display()
function printable(what, numformat)
  local ret

  if type(what) == 'string' then
    ret = "'"..what.."'"
--    ret = string.format("%q", what)    -- this was ugly

  elseif type(what) == 'number' then
    if numformat then ret = string.format(numformat, what)
    else ret = what end

  elseif type(what) == 'boolean' then
    ret = tostring(what)

  elseif type(what) == 'table' then
    ret = what.__customtype or type(what)

  else
    ret = type(what)
--    ret = tostring(what)               -- this was ugly
  end

  return ret
end


-- Handles indentation
do local indents = {}  -- simulate a static variable
function indent(num)

  if not indents[num] then
    indents[num] = ""
    for i = 0, num do
      indents[num] = indents[num].."  "
    end
  end

  return indents[num]
end
end

----------------------------------------------------------------------------------
-- Functions written by Benjamin Smith - December 2009
----------------------------------------------------------------------------------
if package.loaded["rex_pcre"] then rex = require"rex_pcre" end

-- Echo with color function.
-- Arg1: String to echo
-- Arg2: String containing value for foreground color in hexadecimal RGB format
-- Arg3: String containing value for background color in hexadecimal RGB format
-- Arg4: Bool that tells the function to use insertText() rather than echo()
-- Arg5: Name of the console to echo to. Defaults to main.
--
-- Color changes can be made within the string using the format |cFRFGFB,BRBGBB
-- where FR is the foreground red value, FG is the foreground green value, FB
-- is the foreground blue value, BR is the background red value, etc. ,BRBGBB
-- is optional. |r can be used within the string to reset the colors to default.
-- 
-- The colors in arg2 and arg3 replace the normal defaults for your console.
-- So if you use cecho("|cff0000Testing |rTesting", "00ff00", "0000ff"),
-- the first Testing would be red on black and the second would be green on blue.

if rex then
	function checho(str, fgColor, bgColor, insert, win)
		local t = {}
		local reset, out
		if insert then
			if win then
				out = function(win, str)
					insertText(win, str)
				end
			else
				out = function(str)
					insertText(str)
				end
			end
		else
			if win then
				out = function(win, str)
					echo(win, str)
				end
			else
				out = function(str)
					echo(str)
				end
			end
		end
		if win then
			if fgColor then
				if bgColor then
					reset = function()
						local fr, fg, fb = rex.new("([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})"):match(fgColor)
						fr, fg, fb = tonumber(fr, 16), tonumber(fg, 16), tonumber(fb, 16)
						setFgColor(win, fr, fg, fb)
						local br, bg, bb = rex.new("([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})"):match(bgColor)
						br, bg, bb = tonumber(br, 16), tonumber(bg, 16), tonumber(bb, 16)
						setBgColor(win, br, bg, bb)
					end
				else
					reset = function()
						local fr, fg, fb = rex.new("([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})"):match(fgColor)
						fr, fg, fb = tonumber(fr, 16), tonumber(fg, 16), tonumber(fb, 16)
						setFgColor(win, fr, fg, fb)
					end
				end
			elseif bgColor then
				reset = function()
					local br, bg, bb = rex.new("([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})"):match(bgColor)
					br, bg, bb = tonumber(br, 16), tonumber(bg, 16), tonumber(bb, 16)
					setBgColor(win, br, bg, bb)
				end
			else
				reset = function()
					resetFormat(win)
				end
			end
		else
			if fgColor then
				if bgColor then
					reset = function()
						local fr, fg, fb = rex.new("([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})"):match(fgColor)
						fr, fg, fb = tonumber(fr, 16), tonumber(fg, 16), tonumber(fb, 16)
						setFgColor(fr, fg, fb)
						local br, bg, bb = rex.new("([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})"):match(bgColor)
						br, bg, bb = tonumber(br, 16), tonumber(bg, 16), tonumber(bb, 16)
						setBgColor(br, bg, bb)
					end
				else
					reset = function()
						local fr, fg, fb = rex.new("([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})"):match(fgColor)
						fr, fg, fb = tonumber(fr, 16), tonumber(fg, 16), tonumber(fb, 16)
						setFgColor(fr, fg, fb)
					end
				end
			elseif bgColor then
				reset = function()
					local br, bg, bb = rex.new("([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})"):match(bgColor)
					br, bg, bb = tonumber(br, 16), tonumber(bg, 16), tonumber(bb, 16)
					setBgColor(br, bg, bb)
				end
			else
				reset = function()
					resetFormat()
				end
			end
		end
		
		for s, fr, fg, fb, br, bg, bb, r in rex.split(str, [[(?:\|c([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2}),?([0-9a-fA-F]{2})?([0-9a-fA-F]{2})?([0-9a-fA-F]{2})?|(\|r))]]) do 
			local fore, back, color
			if fr and fg and fb then fore = { fr, fg, fb } end
			if br and bg and bb then back = { br, bg, bb } end
			if fore and back then 
				color = { ["fg"] = fore, ["bg"] = back }
			elseif fore then
				color = { ["fg"] = fore }
			end
			if s then table.insert(t, s) end
			if color then table.insert(t, color) end
			if r then table.insert(t, "r") end
		end
		
		for _, v in ipairs(t) do
			if type(v) == 'table' then
				local fr, fg, fb = unpack(v.fg)
				fr, fg, fb = tonumber(fr, 16), tonumber(fg, 16), tonumber(fb, 16)
				if win then setFgColor(win, fr, fg, fb) else setFgColor(fr, fg, fb) end
				if v.bg then
					local br, bg, bb = unpack(v.bg)
					br, bg, bb = tonumber(br, 16), tonumber(bg, 16), tonumber(bb, 16)
					if win then setBgColor(win, br, bg, bb) else setBgColor(br, bg, bb) end
				end
			elseif v == "r" then
				reset()
			else
				if win then out(win, v) else out(v) end
			end
		end
		if win then resetFormat(win) else resetFormat() end
	end
end

-- Enable mathematical operations & comparisons on matches table.
if not matches then matches = {} end
setmetatable( matches, {
	["__add"] = function(op1, op2) 
		local o1, o2 = tonumber(op1), tonumber(op2)
		if (o1 and o2) then
			return (o1 + o2)
		end
	end,
	["__sub"] = function(op1, op2) 
		local o1, o2 = tonumber(op1), tonumber(op2)
		if (o1 and o2) then
			return (o1 - o2)
		end
	end,
	["__div"] = function(op1, op2) 
		local o1, o2 = tonumber(op1), tonumber(op2)
		if (o1 and o2) then
			return (o1 / o2)
		end
	end,
	["__mul"] = function(op1, op2) 
		local o1, o2 = tonumber(op1), tonumber(op2)
		if (o1 and o2) then
			return (o1 * o2)
		end
	end,
	["__mod"] = function(op1, op2) 
		local o1, o2 = tonumber(op1), tonumber(op2)
		if (o1 and o2) then
			return (o1 % o2)
		end
	end,
	["__pow"] = function(op1, op2) 
		local o1, o2 = tonumber(op1), tonumber(op2)
		if (o1 and o2) then
			return (o1 ^ o2)
		end
	end,
	["__unm"] = function(op) 
		local o = tonumber(op)
	    if o then
			return -o
		end
	end,
	["__eq"] = function(op1, op2) 
		local o1, o2 = tonumber(op1), tonumber(op2)
		if (o1 and o2) then
			if (o1 == o2) then
				return true
			else
				return false
			end
		elseif ((o1 and (not o2)) or (o2 and (not o1))) then
				return false
		elseif ((not o1) and (not o2)) then
			if (op1 == op2) then
				return true
			end
		end
	end,
	["__lt"] = function(op1, op2) 
		local o1, o2 = tonumber(op1), tonumber(op2)
		if (o1 and o2) then
			if (o1 < o2) then
				return true
			else
				return false
			end
		elseif ((o1 and (not o2)) or (o2 and (not o1))) then
				return false
		elseif ((not o1) and (not o2)) then
			if (op1 < op2) then
				return true
			end
		end
	end,
	["__le"] = function(op1, op2) 
		local o1, o2 = tonumber(op1), tonumber(op2)
		if (o1 and o2) then
			if (o1 <= o2) then
				return true
			else
				return false
			end
		elseif ((o1 and (not o2)) or (o2 and (not o1))) then
				return false
		elseif ((not o1) and (not o2)) then
			if (op1 <= op2) then
				return true
			end
		end
	end,
	["__gt"] = function(op1, op2) 
		local o1, o2 = tonumber(op1), tonumber(op2)
		if (o1 and o2) then
			if (o1 > o2) then
				return true
			else
				return false
			end
		elseif ((o1 and (not o2)) or (o2 and (not o1))) then
				return false
		elseif ((not o1) and (not o2)) then
			if (op1 > op2) then
				return true
			end
		end
	end,
	["__ge"] = function(op1, op2) 
		local o1, o2 = tonumber(op1), tonumber(op2)
		if (o1 and o2) then
			if (o1 >= o2) then
				return true
			else
				return false
			end
		elseif ((o1 and (not o2)) or (o2 and (not o1))) then
				return false
		elseif ((not o1) and (not o2)) then
			if (op1 >= op2) then
				return true
			end
		end
	end,
	})

