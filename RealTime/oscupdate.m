% oscupdate - send outgoing OSC updates
function info=oscupdate(p,info,sampnum,snap,prevsnap)
  debug=true;
  log=false;
  
  info.sampnum=sampnum;
  if nargin>=4
    info.snap=snap;
  else
    info.snap=[];
  end
  if nargin>=5
    info.prevsnap=prevsnap;
  else
    info.prevsnap=[];
  end
  
  % Just began
  running=true;
  if info.juststarted
    info=info.currapp.fn(p,info,'start');
    oscmsgout(p.oscdests,'/pf/started',{});
    info.refresh=true;   % First call - do a dump
  elseif nargin<4
    info=info.currapp.fn(p,info,'stop');
    oscmsgout(p.oscdests,'/pf/stopped',{});
    running=false;
  end

  if info.refresh
    fprintf('Sending refresh to OSC listeners\n');
    info.refresh=false;
    active=single(p.layout.active);
    oscmsgout(p.oscdests,'/pf/set/minx',{min(active(:,1))});
    oscmsgout(p.oscdests,'/pf/set/maxx',{max(active(:,1))});
    oscmsgout(p.oscdests,'/pf/set/miny',{min(active(:,2))});
    oscmsgout(p.oscdests,'/pf/set/maxy',{max(active(:,2))});

    % Refresh UI
    oscmsgout('TO','/sound/app/name',{info.currapp.name});
    for i=1:length(info.apps)
      oscmsgout('TO',sprintf('/sound/app/buttons/%s',info.apps(i).pos),{i==info.currapp.index});
    end

    oscmsgout('TO','/sample/app/name',{info.currsample.name});
    for i=1:length(info.samples)
      oscmsgout('TO',sprintf('/sample/app/buttons/%s',info.samples(i).pos),{i==info.currsample.index});
    end
    
    if running
      col='green';
    else
      col='red';
    end
    oscmsgout('TO','/sound/app/buttons/color',{col});
    oscmsgout('TO','/sound/app/name/color',{col});
    oscmsgout('TO','/sound/app/title/color',{col});
    oscmsgout('TO','/sample/app/buttons/color',{col});
    oscmsgout('TO','/sample/app/name/color',{col});
    oscmsgout('TO','/sample/app/title/color',{col});

    % Page 2, channel displays
    if nargin>=4
      oscmsgout(p.oscdests,'/pf/set/npeople',{int32(length(snap.hypo))});
    end

    for channel=1:length(info.channels)
      id=info.channels(channel);
      % fprintf('Channel %d, id=%d\n', channel, id);
      if id>0
        oscmsgout('TO',sprintf('/touchosc/loc/%d/visible',channel),{1});
        oscmsgout('TO',sprintf('/touchosc/loc/%d/color',channel),{col2touchosc(id2color(id,p.colors))});
        oscmsgout('TO',sprintf('/touchosc/id/%d',channel),{num2str(id)});
      else
        oscmsgout('TO',sprintf('/touchosc/loc/%d/visible',channel),{0});
        oscmsgout('TO',sprintf('/touchosc/id/%d',channel),{''});
      end
    end

    for i=1:length(info.pgm)
      oscmsgout('TO',sprintf('/midi/pgm/%d/value',i),{info.pgms{info.pgm(i)}});
      oscmsgout('MAX','/pf/pass/pgmout',{int32(i),int32(info.pgm(i))});
    end
    for i=1:15
      if info.preset==16-i
        oscmsgout('TO',sprintf('/midi/preset/%d/1',i),{int32(1)});
      else
        oscmsgout('TO',sprintf('/midi/preset/%d/1',i),{int32(0)});
      end
    end
    if info.preset>0
      oscmsgout('TO','/midi/presetname',{info.presetnames{info.preset}});
    else
      oscmsgout('TO','/midi/presetname',{''});
    end
    oscmsgout('TO','/midi/duration',{info.duration});
    oscmsgout('TO','/midi/duration/value',{info.duration});
    oscmsgout('TO','/midi/velocity',{info.velocity});
    oscmsgout('TO','/midi/velocity/value',{info.velocity});
    oscmsgout('TO','/tempo',{info.tempo});
    oscmsgout('MAX','/pf/pass/transport',{'tempo',info.tempo});
    oscmsgout('AL','/live/tempo',{'tempo',info.tempo});
    oscmsgout('TO','/tempo/value',{info.tempo});
    oscmsgout('TO','/volume',{info.volume});
    oscmsgout('TO','/midi/multichannel',{int32(info.multichannel)});
    oscmsgout('TO','/enable/ableton',{int32(info.ableton)});
    oscmsgout('TO','/enable/max',{int32(info.max)});

    oscmsgout('TO','/touchosc/seq/scale/value',{[num2str(info.scale),': ',info.scales{info.scale}]});
    oscmsgout('TO','/touchosc/seq/key/value',{info.keys{info.key}});
  end

  if running
    elapsed=(snap.when-info.starttime)*24*3600;
    
    ids=[snap.hypo.id];
    if nargin>=5
      previds=[prevsnap.hypo.id];
    else
      previds=[];
    end
    
    % Compute entries, exits
    info.entries=setdiff(ids,previds);
    info.exits=setdiff(previds,ids);
    info.updates=intersect(ids,previds);
    
    if mod(sampnum,20)==0
      oscmsgout(p.oscdests,'/pf/frame',{int32(sampnum) });
    end
    
    for i=info.entries
      oscmsgout(p.oscdests,'/pf/entry',{int32(sampnum),elapsed,int32(i)});
      if ~isempty(find(info.channels==i))
        fprintf('Channel already assigned to id %d -- ignoring duplicate entry\n', i);
        continue;
      end
      channel=find(info.channels==0,1);
      if isempty(channel)
        fprintf('No more channels available to allocate to ID %d\n', i);
      else
        oscmsgout('TO',sprintf('/touchosc/loc/%d/color',channel),{col2touchosc(id2color(i,p.colors))});
        oscmsgout('TO',sprintf('/touchosc/loc/%d/visible',channel),{1});
        oscmsgout('TO',sprintf('/touchosc/id/%d',channel),{num2str(i)});
        info.channels(channel)=i;
        % fprintf('Set id %d to channel %d, channels now = %s\n', i, channel, sprintf('%d ',info.channels));
        if info.channels(end)>0
          keyboard;
        end
      end
    end

    people=length(ids);
    prevpeople=length(previds);
    if people~=prevpeople
      oscmsgout(p.oscdests,'/pf/set/npeople',{int32(people)});
    end
    for i=1:length(snap.hypo)
      h=snap.hypo(i);
      % Compute groupid -- lowest id in group, or 0 if not in a group
      sametnum=find(h.tnum==[snap.hypo.tnum]);
      groupid=snap.hypo(min(sametnum)).id;
      % TODO - should send to all destinations except TO
      oscmsgout('LD','/pf/update',{int32(sampnum), elapsed,int32(h.id),h.pos(1),h.pos(2),h.velocity(1),h.velocity(2),h.majoraxislength,h.minoraxislength,int32(groupid),int32(length(sametnum))});
      xypos=h.pos ./max(abs(p.layout.active));
      channel=id2channel(info,h.id);
      oscmsgout('TO',sprintf('/touchosc/loc/%d',channel),{xypos(2),xypos(1)});
    end

    % Update LCD
    lcdsize=400;
    for i=1:length(snap.hypo)
      pos=snap.hypo(i).pos;
      lp=int32((pos+2.5)*lcdsize/5);
      lp(2)=lcdsize-lp(2);  % Inverted axis
      sz=2;
      lp=lp-sz/2;
      le=lp+sz;
      col=id2color(snap.hypo(i).id,p.colors);
      if all(col==127)
        col=[0 0 0];
      end
      col=int32(col*255);
      oscmsgout('MAX','/pf/pass/lcd',{'frgb',col(1),col(2),col(3)});
      oscmsgout('MAX','/pf/pass/lcd',{'paintoval',lp(1),lp(2),le(1),le(2)});
    end

    info=info.currapp.fn(p, info,'update');

    % Need to handle exits last since app.fn may have needed to know which channel the id was on
    for i=info.exits
      oscmsgout(p.oscdests,'/pf/exit',{int32(sampnum),elapsed,int32(i)});
      channel=id2channel(info,i);
      %        oscmsgout('TO',sprintf('/touchosc/loc/%d/color',channel),{'red'});
      oscmsgout('TO',sprintf('/touchosc/loc/%d/visible',channel),{0});
      oscmsgout('TO',sprintf('/touchosc/id/%d',channel),{''});
      info.channels(channel)=0;
      % fprintf('Removed id %d from channel %d, channels now = %s\n', i, channel, sprintf('%d ',info.channels))
    end
  end
  info.juststarted=false;
end
