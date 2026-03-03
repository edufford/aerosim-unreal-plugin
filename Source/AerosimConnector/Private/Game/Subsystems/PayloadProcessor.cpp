#include "Game/Subsystems/PayloadProcessor.h"
#include "JsonUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Util/MessageHandler.h"
#include "MathUtil.h"

/* json schema of scene graph for documentation purpouses

{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "properties": {
	"entities": {
	  "type": "object",
	  "patternProperties": {
		"^entity_\\d+$": {
		  "type": "array",
		  "items": { "type": "string" }
		}
	  }
	},
	"resources": {
	  "type": "object",
	  "properties": {
		"origin": {
		  "type": "object",
		  "properties": {
			"latitude": { "type": "number" },
			"longitude": { "type": "number" },
			"altitude": { "type": "number" }
		  },
		  "required": ["latitude", "longitude", "altitude"]
		},
		"sim_time": {
		  "type": "object",
		  "properties": {
			"sec": { "type": "integer" },
			"nsec": { "type": "integer" }
		  },
		  "required": ["sec", "nsec"]
		}
	  },
	  "required": ["origin", "sim_time"]
	},
	"components": {
	  "type": "object",
	  "properties": {
		"actor_properties": {
		  "type": "object",
		  "patternProperties": {
			"^entity_\\d+$": {
			  "type": "object",
			  "properties": {
				"actor_name": { "type": "string" },
				"actor_asset": { "type": "string" },
				"parent": { "type": "string" }
			  },
			  "required": ["actor_name", "actor_asset", "parent"]
			}
		  }
		},
		"actor_state": {
		  "type": "object",
		  "patternProperties": {
			"^entity_\\d+$": {
			  "type": "object",
			  "properties": {
				"pose": {
				  "type": "object",
				  "properties": {
					"transform": {
					  "type": "object",
					  "properties": {
						"position": {
						  "type": "object",
						  "properties": {
							"x": { "type": "number" },
							"y": { "type": "number" },
							"z": { "type": "number" }
						  },
						  "required": ["x", "y", "z"]
						},
						"orientation": {
						  "type": "object",
						  "properties": {
							"x": { "type": "number" },
							"y": { "type": "number" },
							"z": { "type": "number" },
							"w": { "type": "number" }
						  },
						  "required": ["x", "y", "z", "w"]
						},
						"scale": {
						  "type": "object",
						  "properties": {
							"x": { "type": "number" },
							"y": { "type": "number" },
							"z": { "type": "number" }
						  },
						  "required": ["x", "y", "z"]
						}
					  },
					  "required": ["position", "orientation", "scale"]
					}
				  },
				  "required": ["transform"]
				}
			  },
			  "required": ["pose"]
			}
		  }
		},
		"sensor": {
		  "type": "object",
		  "patternProperties": {
			"^entity_\\d+$": {
			  "type": "object",
			  "properties": {
				"sensor_name": { "type": "string" },
				"sensor_type": { "type": "string" },
				"sensor_parameters": {
				  "type": "object",
				  "properties": {
					"resolution": {
					  "type": "object",
					  "properties": {
						"x": { "type": "integer" },
						"y": { "type": "integer" }
					  },
					  "required": ["x", "y"]
					},
					"tick_rate": { "type": "number" },
					"fov": { "type": "number" },
					"near_clip": { "type": "number" },
					"far_clip": { "type": "number" }
				  },
				  "required": ["resolution", "tick_rate", "fov", "near_clip", "far_clip"]
				}
			  },
			  "required": ["sensor_name", "sensor_type", "sensor_parameters"]
			}
		  }
		}
	  }
	}
  }
}
*/

bool UPayloadProcessor::ParseJson(const FString& JsonString, FSceneGraph& OutSceneGraph)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{

		// === Parse Entities ===
		if (JsonObject->HasField(TEXT("entities")))
		{
			TSharedPtr<FJsonObject> EntitiesObject = JsonObject->GetObjectField(TEXT("entities"));
			if (EntitiesObject)
			{
				for (auto& EntityPair : EntitiesObject->Values)
				{
					FEntityComponentList EntityComponents;
					TArray<TSharedPtr<FJsonValue>> ComponentsArray = EntityPair.Value->AsArray();

					for (TSharedPtr<FJsonValue> Component : ComponentsArray)
					{
						EntityComponents.Components.Add(Component->AsString());
					}

					OutSceneGraph.Entities.Add(EntityPair.Key, EntityComponents);
				}
			}
		}

		if (JsonObject->HasField(TEXT("resources")))
		{
			// === Parse Resources ===
			TSharedPtr<FJsonObject> ResourcesObject = JsonObject->GetObjectField(TEXT("resources"));
			if (ResourcesObject)
			{
				FResources Resources;
				TSharedPtr<FJsonObject> Origin = ResourcesObject->GetObjectField(TEXT("origin"));
				Resources.Origin = FVector(
					Origin->GetNumberField(TEXT("latitude")),
					Origin->GetNumberField(TEXT("longitude")),
					Origin->GetNumberField(TEXT("altitude")));

				TSharedPtr<FJsonObject> Weather = ResourcesObject->GetObjectField(TEXT("weather"));
				Resources.Weather.Preset = Weather->GetStringField(TEXT("preset"));

				TSharedPtr<FJsonObject> ViewportConfig = ResourcesObject->GetObjectField(TEXT("viewport_config"));
				if(ViewportConfig)
				{
					Resources.ViewportConfig.ActiveViewport = ViewportConfig->GetStringField(TEXT("active_camera"));
					Resources.ViewportConfig.RendererInstanceID = ViewportConfig->GetStringField(TEXT("renderer_instance"));
				}
				Resources.bResourcesSet = true;

				OutSceneGraph.Resources = Resources;
			}
		}

		if (JsonObject->HasField(TEXT("components")))
		{
			// === Parse Components ===
			TSharedPtr<FJsonObject> ComponentsObject = JsonObject->GetObjectField(TEXT("components"));
			if (!ComponentsObject)
				return false;

			if (ComponentsObject->HasField(TEXT("actor_properties")))
			{
				// Parse actor properties
				TSharedPtr<FJsonObject> ActorProperties = ComponentsObject->GetObjectField(TEXT("actor_properties"));
				for (auto& ActorPair : ActorProperties->Values)
				{
					FActorProperties ActorData;
					TSharedPtr<FJsonObject> ActorInfo = ActorPair.Value->AsObject();

					ActorData.ActorName = ActorInfo->GetStringField(TEXT("actor_name"));
					ActorData.ActorAsset = ActorInfo->GetStringField(TEXT("actor_asset"));
					ActorData.Parent = ActorInfo->GetStringField(TEXT("parent"));

					OutSceneGraph.Components.ActorProperties.Add(ActorPair.Key, ActorData);
				}
			}

			if (ComponentsObject->HasField(TEXT("actor_state")))
			{
				// Parse transforms
				TSharedPtr<FJsonObject> ActorState = ComponentsObject->GetObjectField(TEXT("actor_state"));
				for (auto& ActorPair : ActorState->Values)
				{
					FTransformSceneGraph Transform;
					TSharedPtr<FJsonObject> TransformObject = ActorPair.Value->AsObject()->GetObjectField(TEXT("pose"))->GetObjectField(TEXT("transform"));

					Transform.Position = FVector(
						TransformObject->GetObjectField(TEXT("position"))->GetNumberField(TEXT("x")),
						TransformObject->GetObjectField(TEXT("position"))->GetNumberField(TEXT("y")),
						TransformObject->GetObjectField(TEXT("position"))->GetNumberField(TEXT("z")));

					FQuat Aux;
					Aux.X = TransformObject->GetObjectField(TEXT("orientation"))->GetNumberField(TEXT("x"));
					Aux.Y = TransformObject->GetObjectField(TEXT("orientation"))->GetNumberField(TEXT("y"));
					Aux.Z = TransformObject->GetObjectField(TEXT("orientation"))->GetNumberField(TEXT("z"));
					Aux.W = TransformObject->GetObjectField(TEXT("orientation"))->GetNumberField(TEXT("w"));

					if (Aux.Size() < TMathUtilConstants<float>::Epsilon)
					{
						UE_LOG(LogAerosimConnector, Error, TEXT("Invalid quaternion in actor state, setting to identity."));
						Aux = FQuat::Identity;
					}

					Transform.Scale = FVector(
						TransformObject->GetObjectField(TEXT("scale"))->GetNumberField(TEXT("x")),
						TransformObject->GetObjectField(TEXT("scale"))->GetNumberField(TEXT("y")),
						TransformObject->GetObjectField(TEXT("scale"))->GetNumberField(TEXT("z")));

					aerosim_quat_wxyz_to_rpy(
						Aux.W,
						Aux.X,
						Aux.Y,
						Aux.Z,
						&Transform.Rotation.Roll,
						&Transform.Rotation.Pitch,
						&Transform.Rotation.Yaw);

					FActorState State;
					State.Pose = Transform;
					OutSceneGraph.Components.ActorStates.Add(ActorPair.Key, State);
				}
			}

			if (ComponentsObject->HasField(TEXT("sensor")))
			{
				// Parse sensor data
				TSharedPtr<FJsonObject> SensorComponent = ComponentsObject->GetObjectField(TEXT("sensor"));
				for (auto& SensorPair : SensorComponent->Values)
				{
					FSensorData Sensor;
					TSharedPtr<FJsonObject> SensorInfo = SensorPair.Value->AsObject();
					Sensor.SensorName = SensorInfo->GetStringField(TEXT("sensor_name"));
					Sensor.SensorType = SensorInfo->GetStringField(TEXT("sensor_type"));

					TSharedPtr<FJsonObject> SensorParameters = SensorInfo->GetObjectField(TEXT("sensor_parameters"));
					TSharedPtr<FJsonObject> RGBAParams = SensorParameters->GetObjectField(TEXT("RGBCamera"));

					Sensor.TickRate = RGBAParams->GetNumberField(TEXT("tick_rate"));
					Sensor.FOV = RGBAParams->GetNumberField(TEXT("fov"));
					Sensor.NearClip = RGBAParams->GetNumberField(TEXT("near_clip"));
					Sensor.FarClip = RGBAParams->GetNumberField(TEXT("far_clip"));

					if (RGBAParams->HasField(TEXT("projection_type")))
					{
						if (RGBAParams->GetStringField(TEXT("projection_type")).Equals("orthographic"))
						{
							Sensor.ProjectionMode = ECameraProjectionMode::Type::Orthographic;
						}
						else
						{
							Sensor.ProjectionMode = ECameraProjectionMode::Type::Perspective;
						}
					}
					if (RGBAParams->HasField(TEXT("ortographic_width")))
					{
						Sensor.OrthoWidth = RGBAParams->GetNumberField(TEXT("ortographic_width"));
					}
					Sensor.bCaptureEnabled = RGBAParams->GetBoolField(TEXT("capture_enabled"));
					// Extract resolution array
					const TArray<TSharedPtr<FJsonValue>>* ResolutionArray;
					if (RGBAParams->TryGetArrayField(TEXT("resolution"), ResolutionArray) && ResolutionArray->Num() == 2)
					{
						Sensor.Resolution.X = (*ResolutionArray)[0]->AsNumber();
						Sensor.Resolution.Y = (*ResolutionArray)[1]->AsNumber();
					}

					OutSceneGraph.Components.Sensors.Add(SensorPair.Key, Sensor);
				}
			}

			if (ComponentsObject->HasField(TEXT("effectors")))
			{
				// Parse effectors
				TSharedPtr<FJsonObject> EffectorsComponent = ComponentsObject->GetObjectField(TEXT("effectors"));
				for (auto& EffectorPair : EffectorsComponent->Values)
				{
					TArray<FEffectorData> EffectorList;
					for (TSharedPtr<FJsonValue> EffectorValue : EffectorPair.Value->AsArray())
					{
						FEffectorData Effector;
						TSharedPtr<FJsonObject> EffectorObject = EffectorValue->AsObject();

						Effector.EffectorID = EffectorObject->GetStringField(TEXT("effector_id"));
						Effector.USDPath = EffectorObject->GetStringField(TEXT("relative_path"));

						// Store transform
						TSharedPtr<FJsonObject> TransformObject = EffectorObject->GetObjectField(TEXT("pose"))->GetObjectField(TEXT("transform"));
						Effector.Transform.Position = FVector(
							TransformObject->GetObjectField(TEXT("position"))->GetNumberField(TEXT("x")),
							TransformObject->GetObjectField(TEXT("position"))->GetNumberField(TEXT("y")),
							TransformObject->GetObjectField(TEXT("position"))->GetNumberField(TEXT("z")));

						FQuat Aux;
						Aux.X = TransformObject->GetObjectField(TEXT("orientation"))->GetNumberField(TEXT("x"));
						Aux.Y = TransformObject->GetObjectField(TEXT("orientation"))->GetNumberField(TEXT("y"));
						Aux.Z = TransformObject->GetObjectField(TEXT("orientation"))->GetNumberField(TEXT("z"));
						Aux.W = TransformObject->GetObjectField(TEXT("orientation"))->GetNumberField(TEXT("w"));

						if (Aux.Size() < TMathUtilConstants<float>::Epsilon)
						{
							UE_LOG(LogAerosimConnector, Error, TEXT("Invalid quaternion in effector state, setting to identity."));
							Aux = FQuat::Identity;
						}

						Effector.Transform.Scale = FVector(
							TransformObject->GetObjectField(TEXT("scale"))->GetNumberField(TEXT("x")),
							TransformObject->GetObjectField(TEXT("scale"))->GetNumberField(TEXT("y")),
							TransformObject->GetObjectField(TEXT("scale"))->GetNumberField(TEXT("z")));

						aerosim_quat_wxyz_to_rpy(
							Aux.W,
							Aux.X,
							Aux.Y,
							Aux.Z,
							&Effector.Transform.Rotation.Roll,
							&Effector.Transform.Rotation.Pitch,
							&Effector.Transform.Rotation.Yaw);

						EffectorList.Add(Effector);
					}

					if (!OutSceneGraph.Components.Effectors.Contains(EffectorPair.Key))
					{
						OutSceneGraph.Components.Effectors.Add(EffectorPair.Key, FEffectorList());
					}
					OutSceneGraph.Components.Effectors[EffectorPair.Key] = FEffectorList(EffectorList);
				}
			}

			if (ComponentsObject->HasField(TEXT("primary_flight_display_state")))
			{
				// Parse PFD state data
				TSharedPtr<FJsonObject> PFDComponentJSON = ComponentsObject->GetObjectField(TEXT("primary_flight_display_state"));
				for (auto& [Entity, ComponentValue] : PFDComponentJSON->Values)
				{
					TSharedPtr<FJsonObject> ComponentObject = ComponentValue->AsObject();
					TSharedPtr<FJsonObject> PFDDataJSON = ComponentObject->GetObjectField(TEXT("pfd_data"));

					FPrimaryFlightDisplayData PFDState;
					PFDState.AirspeedKts = PFDDataJSON->GetNumberField(TEXT("airspeed_kts"));
					PFDState.TrueAirspeedKts = PFDDataJSON->GetNumberField(TEXT("true_airspeed_kts"));
					PFDState.AltitudeFt = PFDDataJSON->GetNumberField(TEXT("altitude_ft"));
					PFDState.TargetAltitudeFt = PFDDataJSON->GetNumberField(TEXT("target_altitude_ft"));
					PFDState.AltimeterPressureSettingInHg = PFDDataJSON->GetNumberField(TEXT("altimeter_pressure_setting_inhg"));
					PFDState.VerticalSpeedFpm = PFDDataJSON->GetNumberField(TEXT("vertical_speed_fpm"));
					PFDState.PitchDeg = PFDDataJSON->GetNumberField(TEXT("pitch_deg"));
					PFDState.RollDeg = PFDDataJSON->GetNumberField(TEXT("roll_deg"));
					PFDState.SideSlipFps2 = PFDDataJSON->GetNumberField(TEXT("side_slip_fps2"));
					PFDState.HeadingDeg = PFDDataJSON->GetNumberField(TEXT("heading_deg"));
					PFDState.HsiCourseSelectHeadingDeg = PFDDataJSON->GetNumberField(TEXT("hsi_course_select_heading_deg"));
					PFDState.HsiCourseDeviationDeg = PFDDataJSON->GetNumberField(TEXT("hsi_course_deviation_deg"));
					PFDState.HsiMode = PFDDataJSON->GetIntegerField(TEXT("hsi_mode"));
					OutSceneGraph.Components.PrimaryFlightDisplays.Add(Entity, PFDState);
				}
			}
			if (ComponentsObject->HasField(TEXT("trajectory")))
			{
				TSharedPtr<FJsonObject> TrajectorySettingsProperties = ComponentsObject->GetObjectField(TEXT("trajectory"));
				for (auto& TrajectoryPair : TrajectorySettingsProperties->Values)
				{
					TSharedPtr<FJsonObject> Info = TrajectoryPair.Value->AsObject()->GetObjectField(TEXT("parameters"));
					{
						TSharedPtr<FJsonObject> SettingsInfo = Info->GetObjectField(TEXT("settings"));
						FTrajectoryVisualizationSettingsData Settings;
						Settings.DisplayFutureTrajectory = SettingsInfo->GetBoolField(TEXT("display_future_trajectory"));
						Settings.DisplayPastTrajectory = SettingsInfo->GetBoolField(TEXT("display_past_trajectory"));
						Settings.HighlightUserDefinedWaypoints = SettingsInfo->GetBoolField(TEXT("highlight_user_defined_waypoints"));
						Settings.NumberOfFutureWaypoints = SettingsInfo->GetIntegerField(TEXT("number_of_future_waypoints"));
						if (!OutSceneGraph.Components.TrajectoryVisualizationSettings.Contains(TrajectoryPair.Key))
						{
							OutSceneGraph.Components.TrajectoryVisualizationSettings.Add(TrajectoryPair.Key, FTrajectoryVisualizationSettingsData());
						}
						OutSceneGraph.Components.TrajectoryVisualizationSettings[TrajectoryPair.Key] = Settings;
					}
					{
						TSharedPtr<FJsonObject> UserDefinedWaypointsInfo = Info->GetObjectField(TEXT("user_defined_waypoints"));
						TArray<TSharedPtr<FJsonValue>> WaypointsJson = UserDefinedWaypointsInfo->GetArrayField(TEXT("waypoints"));
						FTrajectoryVisualizationWaypointsData Waypoints;
						for (auto& WaypointValue : WaypointsJson)
						{
							if (WaypointValue->Type == EJson::Array)
							{
								TArray<TSharedPtr<FJsonValue>> CoordArray = WaypointValue->AsArray();
								if (CoordArray.Num() == 3)
								{
									const float X = CoordArray[0]->AsNumber();
									const float Y = CoordArray[1]->AsNumber();
									const float Z = CoordArray[2]->AsNumber();
									// Convert from NED to UE5 coordinate system
									Waypoints.Waypoints.Add(FVector(Y * 100.0f, -X * 100.0f, -Z * 100.0f));
								}
								else
								{
									UE_LOG(LogAerosimConnector, Error, TEXT("The waypoint element is not an array of 3 elements"));
								}
							}
							else
							{
								UE_LOG(LogAerosimConnector, Error, TEXT("The waypoint element is not an array"));
							}
						}
						if (Waypoints.Waypoints.Num() > 0)
						{
							if (!OutSceneGraph.Components.TrajectoryVisualizationUserDefinedWaypoints.Contains(TrajectoryPair.Key))
							{
								OutSceneGraph.Components.TrajectoryVisualizationUserDefinedWaypoints.Add(TrajectoryPair.Key, FTrajectoryVisualizationWaypointsData(Waypoints));
							}
							else
							{
								OutSceneGraph.Components.TrajectoryVisualizationUserDefinedWaypoints[TrajectoryPair.Key] = FTrajectoryVisualizationWaypointsData(Waypoints);
							}
						}
					}
					{
						TSharedPtr<FJsonObject> FutureTrajectoryWaypointsInfo = Info->GetObjectField(TEXT("future_trajectory"));
						TArray<TSharedPtr<FJsonValue>> WaypointsJson = FutureTrajectoryWaypointsInfo->GetArrayField(TEXT("waypoints"));
						FTrajectoryVisualizationWaypointsData Waypoints;
						for (auto& WaypointValue : WaypointsJson)
						{
							if (WaypointValue->Type == EJson::Array)
							{
								TArray<TSharedPtr<FJsonValue>> CoordArray = WaypointValue->AsArray();
								if (CoordArray.Num() == 3)
								{
									const float X = CoordArray[0]->AsNumber();
									const float Y = CoordArray[1]->AsNumber();
									const float Z = CoordArray[2]->AsNumber();
									// Convert from NED to UE5 coordinate system
									Waypoints.Waypoints.Add(FVector(Y * 100.0f, -X * 100.0f, -Z * 100.0f));
								}
								else
								{
									UE_LOG(LogAerosimConnector, Error, TEXT("The waypoint element is not an array of 3 elements"));
								}
							}
							else
							{
								UE_LOG(LogAerosimConnector, Error, TEXT("The waypoint element is not an array"));
							}
						}
						if (Waypoints.Waypoints.Num() > 0)
						{
							if (!OutSceneGraph.Components.TrajectoryVisualizationFutureTrajectoryWaypoints.Contains(TrajectoryPair.Key))
							{
								OutSceneGraph.Components.TrajectoryVisualizationFutureTrajectoryWaypoints.Add(TrajectoryPair.Key, FTrajectoryVisualizationWaypointsData(Waypoints));
							}
							else
							{
								OutSceneGraph.Components.TrajectoryVisualizationFutureTrajectoryWaypoints[TrajectoryPair.Key] = FTrajectoryVisualizationWaypointsData(Waypoints);
							}
						}
					}
				}
			}
		}

		return true;
	}
	return false;
}
