#include "Hospital.h"
#include "Isolate.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kHospitalLogger), __VA_ARGS__)

void hospitalizePatient(v8::Local<v8::Object> v8_patient, PatientClenupFn* cleanup_fn) {
  TRACE("hospitalizePatient v8_patient={} cleanup_fn={}", v8_patient, (void*)cleanup_fn);
  auto v8_isolate = v8_patient->GetIsolate();
  auto isolate = CIsolate::FromV8(v8_isolate);
  isolate->Hospital().AcceptPatient(v8_patient, cleanup_fn);
}

static void v8WeakCallback(const v8::WeakCallbackInfo<HospitalRecord>& data) {
  auto v8_isolate = data.GetIsolate();
  auto isolate = CIsolate::FromV8(v8_isolate);
  auto hospital_record = data.GetParameter();
  TRACE("v8WeakCallback data.GetParameter={} v8_isolate={}", (void*)hospital_record, P$(v8_isolate));
  isolate->Hospital().PatientIsAboutToDie(v8_isolate, hospital_record);
}

// --------------------------------------------------------------------------------------------------------------------

CHospital::CHospital(v8::IsolateRef v8_isolate) : m_v8_isolate(v8_isolate) {
  TRACE("CHospital::CHospital {} v8_isolate={}", THIS, P$(v8_isolate));
}

CHospital::~CHospital() {
  TRACE("CHospital::~CHospital {}", THIS);
  // let all patients die
  auto it = m_records.begin();
  while (it != m_records.end()) {
    UnplugPatient(*it);
    it++;
  }
}

void CHospital::AcceptPatient(v8::Local<v8::Object> v8_patient, PatientClenupFn* cleanup_fn) {
  TRACE("CHospital::AcceptPatient {} v8_patient={} cleanup_fn={}", THIS, v8_patient, (void*)cleanup_fn);
  auto record = new HospitalRecord(v8_patient, cleanup_fn);
  m_records.insert(record);
  assert(!record->m_v8_patient.IsWeak());
  record->m_v8_patient.SetWeak(record, &v8WeakCallback, v8::WeakCallbackType::kFinalizer);
}

void CHospital::PatientIsAboutToDie(v8::IsolateRef v8_isolate, HospitalRecord* record) {
  TRACE("CHospital::AcceptPatient {} record={} ", THIS, (void*)record);

  // just a sanity check that the weak callback is related to our isolate
  assert(v8_isolate == m_v8_isolate);
  assert(m_records.find(record) != m_records.end());

  // remove our records
  m_records.erase(record);

  UnplugPatient(record);
}

void CHospital::UnplugPatient(HospitalRecord* record) {
  TRACE("CHospital::UnplugPatient {} record={} ", THIS, (void*)record);

  // call custom cleanup function
  record->m_cleanup_fn(record->m_v8_patient.Get(m_v8_isolate));

  // unplug him
  record->m_v8_patient.Reset();

  delete record;
}
