-- Test mode for LED API
MODE_NAME = "LED Test"
SLIDER_LABELS = {"S1", "S2", "S3", "S4"}

function init(context)
    print("LED Test mode initialized")
end

function process_event(track, event)
    if not event.switch then
        return
    end

    -- Use track number to test different LED patterns
    if track == 0 then
        led("tempo", 255)  -- Full brightness
    elseif track == 1 then
        led("held", 255)
    elseif track == 2 then
        led("saving", 255)
    elseif track == 3 then
        led("loading", 255)
    elseif track == 4 then
        led("error", 255)
    elseif track == 5 then
        led("mirror", 255)
    elseif track == 6 then
        led("saving", 128)  -- Half brightness
    elseif track == 7 then
        led("error", 64)  -- Quarter brightness
    end
end
