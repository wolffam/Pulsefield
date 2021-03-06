%% Motion-Based Multiple Object Tracking
% This example shows how to perform automatic detection and motion-based
% tracking of moving objects in a video from a stationary camera.
%
% Copyright 2012 The MathWorks, Inc.

%%
% Detection of moving objects and motion-based tracking are important 
% components of many computer vision applications, including activity
% recognition, traffic monitoring, and automotive safety.  The problem of
% motion-based object tracking can be divided into two parts:
%
% # detecting moving objects in each frame 
% # associating the detections corresponding to the same object over time
%
% The detection of moving objects uses a background subtraction algorithm
% based on Gaussian mixture models. Morphological operations are applied to
% the resulting foreground mask to eliminate noise. Finally, blob analysis
% detects groups of connected pixels, which are likely to correspond to
% moving objects. 
%
% The association of detections to the same object is based solely on
% motion. The motion of each track is estimated by a Kalman filter. The
% filter is used to predict the track's location in each frame, and
% determine the likelihood of each detection being assigned to each 
% track.
%
% Track maintenance becomes an important aspect of this example. In any
% given frame, some detections may be assigned to tracks, while other
% detections and tracks may remain unassigned.The assigned tracks are
% updated using the corresponding detections. The unassigned tracks are 
% marked invisible. An unassigned detection begins a new track. 
%
% Each track keeps count of the number of consecutive frames, where it
% remained unassigned. If the count exceeds a specified threshold, the
% example assumes that the object left the field of view and it deletes the
% track.  
%
% This example is a function with the main body at the top and helper 
% routines in the form of 
% <matlab:helpview(fullfile(docroot,'toolbox','matlab','matlab_prog','matlab_prog.map'),'nested_functions') nested functions> 
% below.

classdef multiObjectTracking < handle
  properties
    minVisibleCount = 8;
    tracks;
    assignments;	% (N,2) matrix;  (:,1) is centroid index, (:,2) is track ID
    cost;
    unassignedTracks;	% trackIDs for unassigned tracks
    unassignedDetections;	% Centroid indices for unassigned detections
    nextId;
    maxrange;
  end
  
  methods
%% Create System Objects
function obj = multiObjectTracking()
% create system objects used for reading video, detecting moving objects,
% and displaying the results
  obj.initializeTracks(); % create an empty array of tracks
  obj.nextId = 1; % ID of the next track
  obj.maxrange = 7;
end

% Clone this class
function c = clone(obj)
  c=multiObjectTracking();
  c.minVisibleCount=obj.minVisibleCount;
  c.tracks=obj.tracks;
  c.assignments=obj.assignments;
  c.cost=obj.cost;
  c.unassignedTracks=obj.unassignedTracks;
  c.unassignedDetections=obj.unassignedDetections;
  c.nextId=obj.nextId;
  c.maxrange=obj.maxrange;
  for i=1:length(c.tracks)
    c.tracks(i).kalmanFilter=c.tracks(i).kalmanFilter.clone();
  end
end

function update(obj,vis,nsteps,fps)
  centroids=vis.targets.pos;
  legs=vis.targets.legs;
  obj.predictNewLocationsOfTracks(nsteps);
  % Constraints
  obj.constrainVelocity(fps);
  moveintoshadows(obj,vis);
  obj.detectionToTrackAssignment(centroids);
  obj.updateAssignedTracks(centroids,legs);
  obj.updateUnassignedTracks();
  obj.deleteLostTracks();
  obj.createNewTracks(centroids,legs);
  for trackIdx=1:length(obj.tracks)
    obj.tracks(trackIdx).updatedLoc=obj.tracks(trackIdx).kalmanFilter.State([1,3])';
  end
end



%% Initialize Tracks
% The |initializeTracks| function creates an array of tracks, where each
% track is a structure representing a moving object. The
% purpose of the structure is to maintain the state of a tracked object.
% The state consists of information used for detection to track assignment,
% track termination, and display. 
%
% The structure contains the following fields:
%
% * |id| :                  the integer ID of the track
% * |legs| :                the current leg positions
% * |kalmanFilter| :        a Kalman filter object used for motion-based
%                           tracking
% * |age| :                 the number of frames since the track was first
%                           detected
% * |totalVisibleCount| :   the total number of frames in which the track
%                           was detected (visible)
% * |consecutiveInvisibleCount| : the number of consecutive frames for 
%                                  which the track was not detected (invisible).
%
% Noisy detections tend to result in short-lived tracks. For this reason,
% the example only displays an object after it was tracked for some number
% of frames. This happens when |totalVisibleCount| exceeds a specified 
% threshold.    
%
% When no detections are associated with a track for several consecutive
% frames, the example assumes that the object has left the field of view 
% and deletes the track. This happens when |consecutiveInvisibleCount|
% exceeds a specified threshold. A track may also get deleted as noise if 
% it was tracked for a short time, and marked invisible for most of the of 
% the frames.        

function initializeTracks(obj)
% create an empty array of tracks
  obj.tracks = struct(...
      'id', {}, ...
      'legs', {}, ...
      'kalmanFilter', {}, ...
      'age', {}, ...
      'totalVisibleCount', {}, ...
      'consecutiveInvisibleCount', {},...
      'predictedLoc',[],...
      'measuredLoc',[],...
      'updatedLoc',[]...
      );
end

function constrainVelocity(obj,fps)
  MAXSPEED=1.3/fps;       % 1.3 m/s
  for i=1:length(obj.tracks)
    state=obj.tracks(i).kalmanFilter.State;
    spd=norm(state([2,4]));
    if spd>MAXSPEED;
      fprintf('Limiting speed of ID %d to %.1f m/s (was %.1f m/s)\n', obj.tracks(i).id, MAXSPEED*fps, spd*fps);
      state([2,4])=state([2,4])*(MAXSPEED/spd);
      obj.tracks(i).kalmanFilter.State=state;
    end
  end
end

  

%% Predict New Locations of Existing Tracks
% Use the Kalman filter to predict the centroid of each track in the
% current frame, and update its bounding box accordingly.

function predictNewLocationsOfTracks(obj,nsteps)
  if nargin<2
    nsteps=1;
  end
  for i = 1:length(obj.tracks)
    legs = obj.tracks(i).legs;
    
    % predict the current location of the track
    if ~isempty(obj.tracks(i).updatedLoc)
      lastpos=obj.tracks(i).updatedLoc;
    else
      lastpos=obj.tracks(i).predictedLoc;
    end
    for k=1:nsteps
      predictedCentroid = predict(obj.tracks(i).kalmanFilter);
    end
    delta=predictedCentroid-lastpos;
    obj.tracks(i).predictedLoc=predictedCentroid;
    obj.tracks(i).measuredLoc=[];
    obj.tracks(i).updatedLoc=[];

    % shift the legs the same amount
    obj.tracks(i).legs.c1=obj.tracks(i).legs.c1+delta;
    obj.tracks(i).legs.c2=obj.tracks(i).legs.c2+delta;
  end
end

%% Assign Detections to Tracks
% Assigning object detections in the current frame to existing tracks is
% done by minimizing cost. The cost is defined as the negative
% log-likelihood of a detection corresponding to a track.  
%
% The algorithm involves two steps: 
%
% Step 1: Compute the cost of assigning every detection to each track using
% the |distance| method of the |vision.KalmanFilter| System object. The 
% cost takes into account the Euclidean distance between the predicted
% centroid of the track and the centroid of the detection. It also includes
% the confidence of the prediction, which is maintained by the Kalman
% filter. The results are stored in an MxN matrix, where M is the number of
% tracks, and N is the number of detections.   
%
% Step 2: Solve the assignment problem represented by the cost matrix using
% the |assignDetectionsToTracks| function. The function takes the cost 
% matrix and the cost of not assigning any detections to a track.  
%
% The value for the cost of not assigning a detection to a track depends on
% the range of values returned by the |distance| method of the 
% |vision.KalmanFilter|. This value must be tuned experimentally. Setting 
% it too low increases the likelihood of creating a new track, and may
% result in track fragmentation. Setting it too high may result in a single 
% track corresponding to a series of separate moving objects.   
%
% The |assignDetectionsToTracks| function uses the Munkres' version of the
% Hungarian algorithm to compute an assignment which minimizes the total
% cost. It returns an M x 2 matrix containing the corresponding indices of
% assigned tracks and detections in its two columns. It also returns the
% indices of tracks and detections that remained unassigned. 

function detectionToTrackAssignment(obj,centroids)
  nTracks = length(obj.tracks);
  nDetections = size(centroids, 1);
  
  % compute the cost of assigning each detection to each track
  cost = zeros(nTracks, nDetections);
  %  fprintf('Cost=\n');
  if nDetections>0
    for i = 1:nTracks
      %cost(i, :) = distance(obj.tracks(i).kalmanFilter, centroids);
      % kalman distance bounces around way too much...
      cost(i,:)=sqrt((obj.tracks(i).predictedLoc(1)-centroids(:,1)).^2+(obj.tracks(i).predictedLoc(2)-centroids(:,2)).^2);
      %   fprintf('%2d %s\n',obj.tracks(i).id,sprintf('%4.1f ',cost(i,:)));
    end
  end
  reliableTrackInds =  [obj.tracks(:).totalVisibleCount] > obj.minVisibleCount;
  
  cost(~reliableTrackInds,:)=cost(~reliableTrackInds,:)+.5;   % Increase cost of assigning to an invisible track
                                                             % If this is too big, the cost > cost of unassigned tracks so tracks never stay visible for long
                                                             % If too small, then ephemeral tracks take over long-lived ones
  obj.cost=cost;
  
  % solve the assignment problem
  costUnassignedTracks = .5;
  costUnassignedDetections = .5;
  [obj.assignments, obj.unassignedTracks, obj.unassignedDetections] = ...
      assignDetectionsToTracks(cost, costUnassignedTracks, costUnassignedDetections);
  % Change from track indices to track ids
  if ~isempty(obj.assignments)
    obj.assignments(:,1)=[obj.tracks(obj.assignments(:,1)).id];
  end
  if ~isempty(obj.unassignedTracks)
    obj.unassignedTracks=[obj.tracks(obj.unassignedTracks).id];
  end
end

%% Update Assigned Tracks
% The |updateAssignedTracks| function updates each assigned track with the
% corresponding detection. It calls the |correct| method of
% |vision.KalmanFilter| to correct the location estimate. Next, it stores
% the new bounding box, and increases the age of the track and the total
% visible count by 1. Finally, the function sets the invisible count to 0. 

function updateAssignedTracks(obj,centroids,alllegs)
  numAssignedTracks = size(obj.assignments, 1);
  for i = 1:numAssignedTracks
    trackIdx = find([obj.tracks.id]==obj.assignments(i, 1));
    detectionIdx = obj.assignments(i, 2);
    centroid = centroids(detectionIdx, :);
    legs = alllegs(detectionIdx);
    
    % correct the estimate of the object's location
    % using the new detection
    correct(obj.tracks(trackIdx).kalmanFilter, centroid);

    obj.tracks(trackIdx).measuredLoc=centroid;
    
    % replace predicted bounding box with detected
    % bounding box
    obj.tracks(trackIdx).legs = legs;
    
    % update track's age
    obj.tracks(trackIdx).age = obj.tracks(trackIdx).age + 1;
    
    % update visibility
    obj.tracks(trackIdx).totalVisibleCount = ...
        obj.tracks(trackIdx).totalVisibleCount + 1;
    obj.tracks(trackIdx).consecutiveInvisibleCount = 0;
  end
end

%% Update Unassigned Tracks
% Mark each unassigned track as invisible, and increase its age by 1.

function updateUnassignedTracks(obj)
  for i = 1:length(obj.unassignedTracks)
    ind = find([obj.tracks.id]==obj.unassignedTracks(i));
    obj.tracks(ind).age = obj.tracks(ind).age + 1;
    obj.tracks(ind).consecutiveInvisibleCount = ...
        obj.tracks(ind).consecutiveInvisibleCount + 1;
    obj.tracks(ind).measuredLoc=[];
  end
end

%% Delete Lost Tracks
% The |deleteLostTracks| function deletes tracks that have been invisible
% for too many consecutive frames. It also deletes recently created tracks
% that have been invisible for too many frames overall. 

function deleteLostTracks(obj)
  if isempty(obj.tracks)
    return;
  end
  
  invisibleForTooLong = 50;
  ageThreshold = 20;
  
  % compute the fraction of the track's age for which it was visible
  ages = [obj.tracks(:).age];
  totalVisibleCounts = [obj.tracks(:).totalVisibleCount];
  visibility = totalVisibleCounts ./ ages;
  
  % find the indices of 'lost' tracks
  lostInds = (ages < ageThreshold & visibility < 0.6) | ...
      [obj.tracks(:).consecutiveInvisibleCount] >= invisibleForTooLong;
  outsideInds = arrayfun(@(z) ~isempty(z.predictedLoc) && (norm(z.predictedLoc) > obj.maxrange || z.predictedLoc(2)<0),obj.tracks);
  if sum(outsideInds)>0
    fprintf('Deleting %d tracks out of range\n', sum(outsideInds));
  end
  fl=find(lostInds|outsideInds);
  for i=1:length(fl)
    t=obj.tracks(fl(i));
    fprintf('Deleting track %d with age %d, total %d, consec invis %d\n', ...
            t.id,t.age, t.totalVisibleCount,t.consecutiveInvisibleCount);
  end

  % delete lost tracks
  obj.tracks = obj.tracks(~(lostInds|outsideInds));
end

%% Create New Tracks
% Create new tracks from unassigned detections. Assume that any unassigned
% detection is a start of a new track. In practice, you can use other cues
% to eliminate noisy detections, such as size, location, or appearance.

function createNewTracks(obj,centroids,alllegs)
  centroids = centroids(obj.unassignedDetections, :);
  alllegs = alllegs(obj.unassignedDetections);
  
  for i = 1:size(centroids, 1)
    
    centroid = centroids(i,:);
    legs = alllegs(i);
    
    fprintf('Create new track with id %d at (%.1f, %.1f)\n', obj.nextId, centroid);
    % create a Kalman filter object
    % Initial estimate is accurate for position, but unknown for velocity
    % Use observed state matrix vairances from diagnostic() output to estimate these
    % initial position error = same as state position error
    % initial velocity error = MSE(velocity)=MSE(KF.State([2,4]).^2)
    % state position error = MSE(KF.StateCovariance([1,3]))
    % state velocity error = MSE(KF.StateCovariance([2,4]))
    kalmanFilter = configureKalmanFilter('ConstantVelocity', ...
                                         centroid, [0.2, 0.05].^2, [0.05, 0.01].^2, 0.2^2);
    
    % create a new track
    newTrack = struct(...
        'id', obj.nextId, ...
        'legs', legs, ...
        'kalmanFilter', kalmanFilter, ...
        'age', 1, ...
        'totalVisibleCount', 1, ...
        'consecutiveInvisibleCount', 0, ...
        'predictedLoc',[],...
        'measuredLoc',centroid,...
        'updatedLoc',centroid...
        );
    
    % add it to the array of tracks
    obj.tracks(end + 1) = newTrack;
    
    % And to the assignments
    obj.assignments(end+1,:)=[obj.nextId,obj.unassignedDetections(i)];

    % increment the next id
    obj.nextId = obj.nextId + 1;
  end
end
end
end