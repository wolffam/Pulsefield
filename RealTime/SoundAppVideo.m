% Video app -- primarily just at NO-OP so the video app can take control
classdef SoundAppVideo < SoundApp
  properties

  end
  
  methods

    function obj=SoundAppVideo(uiposition)
      obj=obj@SoundApp(uiposition);
      obj.name='Video';
    end

  end
end
