% Diagnostic plots/output
function diagnostic(snap,varargin)
defaults=struct('trackid',[],...   % Only show these trackids
                'frames',[],...
                'debug',false...
                );
args=processargs(defaults,varargin);

if ~isempty(args.frames)
  frame=arrayfun(@(z) z.vis.frame, snap);
  i1=find(frame>=args.frames(1),1);
  i2=find(frame<=args.frames(2),1,'last');
  snap=snap(i1:i2);
  fprintf('Showing snap(%d:%d)\n', i1, i2);
end
frame=arrayfun(@(z) z.vis.frame,snap);

if length(snap)>1
  % Calculate tracker stats
  alls=[];allc=[];
  for i=1:length(snap)
    t=snap(i).tracker;
    for j=1:length(t.tracks)
      if t.tracks(j).age>10 && (isempty(args.trackid) || ismember(t.tracks(j).id,args.trackid))
        k=t.tracks(j).kalmanFilter;
        alls(:,end+1)=k.State;
        allc(:,end+1)=diag(k.StateCovariance);
      end
    end
  end
  fprintf('Tracker state position sigma: state:(%.2f, %.2f, %.2f, %.2f), cov:(%.2f, %.2f, %.2f, %.2f)\n', sqrt(mean(alls.^2,2)),sqrt(mean(allc,2)));
  setfig('statecov');clf;
  plot(sqrt(allc)');
  legend('px','vx','py','vy');
  ylabel('Sigma');
  xlabel('Sample');
end

bg=snap(end).bg;
vis=snap(end).vis;
tracker=snap(end).tracker;
MAXSPECIAL=2;
fprintf('\n');
setfig('diagnostic');clf;
hold on;

xy=range2xy(vis.angle,vis.range);
bxy=range2xy(bg.angle,bg.range(1,:));

col='gbcymk';
plotted=false(size(vis.class));
for i=1:length(tracker.tracks)
  t=tracker.tracks(i);
  k=t.kalmanFilter;
  loc=t.updatedLoc;
  vel=k.State([2,4])';
  if ~isempty(t.predictedLoc) && ~isempty(t.measuredLoc)
    error=norm(t.predictedLoc-t.measuredLoc);
  else
    error=nan;
  end
  fprintf('Track %d: MSE=%.3f age=%d, visCount=%d, consInvis=%d, loc=(%.2f,%.2f), velocity=(%.2f,%.2f), legs=(%.2f,(%.2f,%.2f),(%.2f,%.2f))\n', t.id, sqrt(mean(error.^2)), t.age, t.totalVisibleCount, t.consecutiveInvisibleCount, t.updatedLoc, vel, t.legs.radius,t.legs.c1,t.legs.c2);
  color=col(min(i,length(col)));
  % plot(t.updatedLoc(1),t.updatedLoc(2),['+',color]);
  if ~isempty(t.predictedLoc)
    plot(t.predictedLoc(1),t.predictedLoc(2),['x',color]);
  end
  if ~isempty(t.measuredLoc)
    plot(t.measuredLoc(1),t.measuredLoc(2),['o',color]);
  end
  asel=tracker.assignments(:,1)==t.id;
  if sum(asel)==1
    det=tracker.assignments(asel,2);
    cnum=det+2;
    fprintf('det=%d, class=%d, npts=%d\n',det,cnum,sum(vis.class==cnum));
    plot(vis.targets.pos(det,1),vis.targets.pos(det,2),['x',color]);
    sel=vis.class==cnum;
    lsel=sel& (vis.leg==2);
    rsel=sel& (vis.leg==1);
    osel=sel&~lsel&~rsel;
    plot(xy(osel,1),xy(osel,2),['.',color]);
    plot(xy(lsel,1),xy(lsel,2),['<',color]);
    plot(xy(rsel,1),xy(rsel,2),['>',color]);
    plotted=plotted|sel;
    % Draw legs
    legs=vis.targets.legs(det);
    angle=-pi:pi/20:pi;
    [x,y]=pol2cart(angle,legs.radius);
    x1=x+legs.c1(1);
    y1=y+legs.c1(2);
    plot(x1,y1,color);
    x2=x+legs.c2(1);
    y2=y+legs.c2(2);
    plot(x2,y2,color);
  end
end
if sum(~plotted)>0
  sel=~plotted & vis.class>MAXSPECIAL;
  if sum(sel)>0
    fprintf('%d target points not matched to tracks\n', sum(sel));
  end
  plot(xy(sel,1),xy(sel,2),'.r');
  sel=~plotted & vis.class>0&vis.class<=MAXSPECIAL;
  plot(xy(sel,1),xy(sel,2),'.k');
end

plot(bxy(:,1),bxy(:,2),'k');
axis equal;
xyt=xy(vis.class>MAXSPECIAL,:);
if ~isempty(xyt)
  c=[min(xyt(:,1)),max(xyt(:,1)),min(xyt(:,2)),max(xyt(:,2))];
  ctr=(floor(c([1,3]))+ceil(c([2,4])))/2;
  sz=max(ceil(c([2,4]))-floor(c([1,3])));
  newc=[ctr(1)-sz/2,ctr(1)+sz/2,ctr(2)-sz/2,ctr(2)+sz/2];
  axis(newc);  % Zoom to ROI
end

if length(snap)>1
  % Plot locations
  setfig('diagnostic-tracks');clf;
  ids=[];
  for i=1:length(snap)
    ids=unique([ids,[snap(i).tracker.tracks.id]]);
  end
  subplot(234);
  for i=1:length(ids)
    idpresent=arrayfun(@(z) ismember(ids(i),[z.tracker.tracks.id]), snap);
    % Only present if there was also a measurement 
    for k=1:length(idpresent)
      if idpresent(k) 
        idpresent(k)=~isempty(snap(k).tracker.tracks([snap(k).tracker.tracks.id]==ids(i)).measuredLoc);
      end
    end
    idtmp=nan(1,length(snap));
    idtmp(idpresent)=ids(i);
    if ismember(ids(i),args.trackid)
      plot(frame,idtmp,'g');
    else
      plot(frame,idtmp,'r');
    end
    hold on;
  end
  ylabel('ID Presence');
  
  for i=1:length(ids)
    id=ids(i);
    if ~isempty(args.trackid) && ~ismember(id,args.trackid)
      continue;
    end
    ploc=nan(length(snap),2);
    mloc=ploc;
    uloc=ploc;
    vel=ploc;
    for j=1:length(snap)
      sel=[snap(j).tracker.tracks.id]==id;
      if sum(sel)>0
        uloc(j,:)=snap(j).tracker.tracks(sel).updatedLoc;
        if ~isempty(snap(j).tracker.tracks(sel).predictedLoc)
          ploc(j,:)=snap(j).tracker.tracks(sel).predictedLoc;
        end
        if ~isempty(snap(j).tracker.tracks(sel).measuredLoc)
          mloc(j,:)=snap(j).tracker.tracks(sel).measuredLoc;
        end
        vel(j,:)=snap(j).tracker.tracks(sel).kalmanFilter.State([2,4]);
      end
    end
    subplot(231);
    plot(ploc(:,1),ploc(:,2),'b.-');
    hold on;
    plot(mloc(:,1),mloc(:,2),'g.-');
    %plot(uloc(:,1),uloc(:,2),'r-');
    %    for k=1:size(ploc,1)
    %      plot([mloc(k,1),ploc(k,1)],[mloc(k,2),ploc(k,2)],'k');
    %    end
    %legend('predicted','measured','Location','SouthOutside');
    
    subplot(232);
    plot(frame,ploc(:,1),'b.-');
    hold on;
    plot(frame,mloc(:,1),'g.-');
    xlabel('Frame');
    title('X Position');
    
    subplot(235)
    plot(frame,ploc(:,2),'b.-');
    hold on;
    plot(frame,mloc(:,2),'g.-');
    xlabel('Frame');
    title('Y Position');

    subplot(233);
    [heading,speed]=cart2pol(vel(:,1),vel(:,2));
    plot(frame,heading*180/pi,'b.-');
    hold on;
    xlabel('Frame');
    title('Heading');
    
    subplot(236)
    plot(frame,speed,'b.-');
    hold on;
    xlabel('Frame');
    title('Speed');

  end
end

      
