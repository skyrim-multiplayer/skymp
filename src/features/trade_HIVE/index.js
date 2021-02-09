import React from 'react';

import { connect } from 'react-redux';

import './styles.sass';

import icons from './icons';

const TradeHIVE = (props) => {
  return (
    props.show &&
    <div id="trade">
      <div className="trade__inner">
        <div className="trade__body">
          <div className="trade__inventory">
            <span className="trade__title">Ваш инвентарь:</span>
            <div className="inventory__tabs">
              <div className="tab active">
                { icons.tabs.favorite }
              </div>
              <div className="tab">
                { icons.tabs.all }
              </div>
              <div className="tab">
                { icons.tabs.weapons }
              </div>
              <div className="tab">
                { icons.tabs.armor }
              </div>
              <div className="tab">
                { icons.tabs.alchemy }
              </div>
              <div className="tab">
                { icons.tabs.scrolls }
              </div>
              <div className="tab">
                { icons.tabs.eat }
              </div>
              <div className="tab">
                { icons.tabs.ingredients }
              </div>
              <div className="tab">
                { icons.tabs.books }
              </div>
              <div className="tab">
                { icons.tabs.materials }
              </div>
            </div>
            <div className="trade__divider"></div>
            <div className="inventory__list">
              <div className="item">
                <div className="item__icon">{ icons.items.sword }</div>
                <span className="item__name">Орихалковый меч</span>
                <div className="item__additional">{ icons.additional.lightning }</div>
                <div className="dropdown">
                  <div className="dropdown__inner">
                    <span className="dropdown__title">Орихалковый меч</span>
                    <div className="dropdown__divider"></div>
                    <div className="dropdown__info">
                      <div className="info__item">
                        Тип&nbsp;<span className="value">Меч</span>
                      </div>
                      <div className="info__item">
                        Урон&nbsp;<span className="value">12</span>
                      </div>
                      <div className="info__item">
                        Вес&nbsp;<span className="value">32</span>
                      </div>
                    </div>
                    <div className="dropdown__additional">Наносит <span className="value">20</span> единиц урона огнем</div>
                  </div>
                </div>
              </div>
              <div className="item">
                <div className="item__icon">{ icons.items.armor }</div>
                <span className="item__name">Железная броня</span>
              </div>
              <div className="item">
                <div className="item__icon">{ icons.items.crossbow }</div>
                <span className="item__name">Арбалет Стражи Рассвета</span>
                <div className="item__additional">{ icons.additional.favorite }</div>
              </div>
              <div className="item">
                <div className="item__icon">{ icons.items.potion }</div>
                <span className="item__name">Зелье лечения (24)</span>
              </div>
              <div className="item">
                <div className="item__icon">{ icons.items.sword }</div>
                <span className="item__name">Орихалковый меч</span>
                <div className="item__additional">{ icons.additional.lightning }</div>
              </div>
              <div className="item">
                <div className="item__icon">{ icons.items.armor }</div>
                <span className="item__name">Железная броня</span>
              </div>
              <div className="item">
                <div className="item__icon">{ icons.items.crossbow }</div>
                <span className="item__name">Арбалет Стражи Рассвета</span>
                <div className="item__additional">{ icons.additional.favorite }</div>
              </div>
              <div className="item">
                <div className="item__icon">{ icons.items.potion }</div>
                <span className="item__name">Зелье лечения (24)</span>
              </div>
              <div className="item">
                <div className="item__icon">{ icons.items.sword }</div>
                <span className="item__name">Орихалковый меч</span>
                <div className="item__additional">{ icons.additional.lightning }</div>
              </div>
              <div className="item">
                <div className="item__icon">{ icons.items.armor }</div>
                <span className="item__name">Железная броня</span>
              </div>
              <div className="item">
                <div className="item__icon">{ icons.items.crossbow }</div>
                <span className="item__name">Арбалет Стражи Рассвета</span>
                <div className="item__additional">{ icons.additional.favorite }</div>
              </div>
              <div className="item">
                <div className="item__icon">{ icons.items.potion }</div>
                <span className="item__name">Зелье лечения (24)</span>
              </div>
              <div className="item">
                <div className="item__icon">{ icons.items.sword }</div>
                <span className="item__name">Орихалковый меч</span>
                <div className="item__additional">{ icons.additional.lightning }</div>
              </div>
              <div className="item">
                <div className="item__icon">{ icons.items.armor }</div>
                <span className="item__name">Железная броня</span>
              </div>
              <div className="item">
                <div className="item__icon">{ icons.items.crossbow }</div>
                <span className="item__name">Арбалет Стражи Рассвета</span>
                <div className="item__additional">{ icons.additional.favorite }</div>
              </div>
              <div className="item">
                <div className="item__icon">{ icons.items.potion }</div>
                <span className="item__name">Зелье лечения (24)</span>
              </div>
            </div>
          </div>
          <div className="trade__area">
            <div className="suggestion">
              <div className="suggestion__header">
                { icons.interface.close }
                <span className="suggestion__person">Вы&nbsp;</span>
                предлагаете:
              </div>
              <div className="trade__divider"></div>
              <div className="suggestion__list">
                <div className="item">
                  <div className="item__icon">{ icons.items.sword }</div>
                  <span className="item__name">Орихалковый меч</span>
                  <div className="item__additional">{ icons.additional.lightning }</div>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.armor }</div>
                  <span className="item__name">Железная броня</span>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.crossbow }</div>
                  <span className="item__name">Арбалет Стражи Рассвета</span>
                  <div className="item__additional">{ icons.additional.favorite }</div>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.potion }</div>
                  <span className="item__name">Зелье лечения (24)</span>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.sword }</div>
                  <span className="item__name">Орихалковый меч</span>
                  <div className="item__additional">{ icons.additional.lightning }</div>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.armor }</div>
                  <span className="item__name">Железная броня</span>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.crossbow }</div>
                  <span className="item__name">Арбалет Стражи Рассвета</span>
                  <div className="item__additional">{ icons.additional.favorite }</div>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.potion }</div>
                  <span className="item__name">Зелье лечения (24)</span>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.sword }</div>
                  <span className="item__name">Орихалковый меч</span>
                  <div className="item__additional">{ icons.additional.lightning }</div>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.armor }</div>
                  <span className="item__name">Железная броня</span>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.crossbow }</div>
                  <span className="item__name">Арбалет Стражи Рассвета</span>
                  <div className="item__additional">{ icons.additional.favorite }</div>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.potion }</div>
                  <span className="item__name">Зелье лечения (24)</span>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.sword }</div>
                  <span className="item__name">Орихалковый меч</span>
                  <div className="item__additional">{ icons.additional.lightning }</div>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.armor }</div>
                  <span className="item__name">Железная броня</span>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.crossbow }</div>
                  <span className="item__name">Арбалет Стражи Рассвета</span>
                  <div className="item__additional">{ icons.additional.favorite }</div>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.potion }</div>
                  <span className="item__name">Зелье лечения (24)</span>
                </div>
              </div>
            </div>
            <div className="suggestion">
              <div className="suggestion__header">
                { icons.interface.open }
                <span className="suggestion__person">Генерал Тулий&nbsp;</span>
                предлагает:
              </div>
              <div className="trade__divider"></div>
              <div className="suggestion__list">
                <div className="item">
                  <div className="item__icon">{ icons.items.sword }</div>
                  <span className="item__name">Орихалковый меч</span>
                  <div className="item__additional">{ icons.additional.lightning }</div>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.armor }</div>
                  <span className="item__name">Железная броня</span>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.crossbow }</div>
                  <span className="item__name">Арбалет Стражи Рассвета</span>
                  <div className="item__additional">{ icons.additional.favorite }</div>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.potion }</div>
                  <span className="item__name">Зелье лечения (24)</span>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.sword }</div>
                  <span className="item__name">Орихалковый меч</span>
                  <div className="item__additional">{ icons.additional.lightning }</div>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.armor }</div>
                  <span className="item__name">Железная броня</span>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.crossbow }</div>
                  <span className="item__name">Арбалет Стражи Рассвета</span>
                  <div className="item__additional">{ icons.additional.favorite }</div>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.potion }</div>
                  <span className="item__name">Зелье лечения (24)</span>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.sword }</div>
                  <span className="item__name">Орихалковый меч</span>
                  <div className="item__additional">{ icons.additional.lightning }</div>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.armor }</div>
                  <span className="item__name">Железная броня</span>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.crossbow }</div>
                  <span className="item__name">Арбалет Стражи Рассвета</span>
                  <div className="item__additional">{ icons.additional.favorite }</div>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.potion }</div>
                  <span className="item__name">Зелье лечения (24)</span>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.sword }</div>
                  <span className="item__name">Орихалковый меч</span>
                  <div className="item__additional">{ icons.additional.lightning }</div>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.armor }</div>
                  <span className="item__name">Железная броня</span>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.crossbow }</div>
                  <span className="item__name">Арбалет Стражи Рассвета</span>
                  <div className="item__additional">{ icons.additional.favorite }</div>
                </div>
                <div className="item">
                  <div className="item__icon">{ icons.items.potion }</div>
                  <span className="item__name">Зелье лечения (24)</span>
                </div>
              </div>
            </div>
          </div>
        </div>
        <div className="trade__footer">
          <div className="trade__actions">
            <div className="action">
              <div className="action__button">
                <div className="button__inner">Y</div>
              </div>
              Принять обмен
            </div>
            <div className="action active">
              <div className="action__button">
                <div className="button__inner">N</div>
              </div>
              Отменить
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

const mapStateToProps = (state) => {
  const defaultState = state.watermarkHiveReducer;

  return {
    show: defaultState.show
  }
}

const mapDispatchToProps = (dispatch) => ({})

export default connect(mapStateToProps, mapDispatchToProps)(TradeHIVE);